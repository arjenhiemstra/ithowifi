#include "i2c_sniffer.h"

#define I2C_SAFE_GAURD_TIMER 15 // delay in ms to execute safe guard commands

typedef enum
{
    ACK = 0x100,
    START = 0x200,
    STOP = 0x400
} i2c_state_t;

typedef uint16_t gpdata_t;
static QueueHandle_t gpio_evt_queue;

#define SCL(pin) (1 << pin)
#define SDA(pin) (1 << pin)
#define GPIO_in GPIO.in

// I2C sniffer pins and safe guard now managed by I2CManager (see managers/I2CManager.cpp)
// Removed: sniffer pin globals, i2c_safe_guard

#include "managers/I2CManager.h"

Ticker i2c_sniffer_timer;

static uint32_t last = 0; // Initialized dynamically at runtime
static uint32_t cur;
static uint32_t bits;
static uint32_t state;
static uint32_t tm1, tm2, tm;
char i2c_header_buf[5]{};

static inline IRAM_ATTR void enable_sda_intr(bool en)
{
    // gpio_set_intr_type() is not IRAM, i.e. too slow
    GPIO.pin[i2cManager.sniffer_sda_pin].int_type = en ? GPIO_INTR_ANYEDGE : GPIO_INTR_DISABLE;
}

static inline IRAM_ATTR uint32_t read_sda_scl_pins()
{
    // gpio_get_level() is not IRAM, i.e. too slow
    return GPIO_in & (SCL(i2cManager.sniffer_scl_pin) | SDA(i2cManager.sniffer_sda_pin));
}

// SCL=1, SDA 1>0 = Start
// SCL=1, SDA 0>1 = End
// SCL 0>1, SDA = data
static void IRAM_ATTR gpio_sda_handler(void *arg)
{
    uint32_t st = read_sda_scl_pins();
    if (last == (SCL(i2cManager.sniffer_scl_pin) | SDA(i2cManager.sniffer_sda_pin)) && st == SCL(i2cManager.sniffer_scl_pin))
    {
        enable_sda_intr(false);
        state = START;
        cur = bits = 0;
        tm1 = tm2 = esp_timer_get_time();
        do
        {
            last = st;
            st = read_sda_scl_pins();
            tm = esp_timer_get_time();
            if (last == SCL(i2cManager.sniffer_scl_pin) && st == (SCL(i2cManager.sniffer_scl_pin) | SDA(i2cManager.sniffer_sda_pin)))
            {
                state = STOP | ((tm - tm1) << 16);
                xQueueSendFromISR(gpio_evt_queue, &state, NULL);
                break;
            }
            else if ((st & SCL(i2cManager.sniffer_scl_pin)) && !(last & SCL(i2cManager.sniffer_scl_pin)))
            {
                if (++bits < 9)
                    cur = (cur << 1) | !!(st & SDA(i2cManager.sniffer_sda_pin));
                else
                {
                    cur |= state | ((st & SDA(i2cManager.sniffer_sda_pin)) ? 0 : ACK) | ((tm - tm2) << 16);
                    xQueueSendFromISR(gpio_evt_queue, &cur, NULL);
                    state = bits = cur = 0;
                    tm2 = tm;
                }
            }
        } while (tm - tm1 < 14900);
        enable_sda_intr(true);
    }
    last = st;
}

static void sniffer_task(void *arg)
{
    int log_entry_index = 0;

    uint32_t x;
    uint32_t flag = 0;
    uint32_t tm1 = esp_timer_get_time(), tm;

    // Initialize last with actual pin values
    last = SCL(i2cManager.sniffer_scl_pin) | SDA(i2cManager.sniffer_sda_pin);

    gpio_evt_queue = xQueueCreate(256, sizeof(uint32_t));
    gpio_install_isr_service(ESP_INTR_FLAG_LEVEL3);
    gpio_config_t config = {BIT64(i2cManager.sniffer_scl_pin) | BIT64(i2cManager.sniffer_sda_pin), GPIO_MODE_INPUT, GPIO_PULLUP_DISABLE, GPIO_PULLDOWN_DISABLE,
                            GPIO_INTR_DISABLE};
    gpio_config(&config);
    gpio_isr_handler_add(i2cManager.sniffer_sda_pin, gpio_sda_handler, (void *)i2cManager.sniffer_sda_pin);
    enable_sda_intr(!!arg);

    std::string buffer;

    for (;;)
    {

        if (xQueueReceive(gpio_evt_queue, &x, portMAX_DELAY))
        {
            tm = esp_timer_get_time();
            if (flag && (tm - tm1) > 200000)
            {
                if (log_entry_index)
                {
                    i2cLogger.i2cLogFinal(log_entry_index, I2CLogger::I2C_OK);
                    log_entry_index = 0;
                }
                i2cSnifferProcessBuf(buffer);
            }
            tm1 = tm;
            flag = 1;
            if (x & START)
            {
                i2cSnifferProcessBuf(buffer);
                buffer += "[";
            }
            if (x & STOP)
            {
#if I2C_SNIFFER_PRINT_TIMING
                char buf[40]{};
                snprintf(buf, sizeof(buf), "]/%d\n", (x >> 16));
                printf(buf);
                buffer += buf;
#else
                buffer += "]";
#endif
                if (log_entry_index)
                {
                    i2cLogger.i2cLogFinal(log_entry_index, I2CLogger::I2C_OK);
                    log_entry_index = 0;
                }
            }
            else
            {
                char buf[50]{};
#if I2C_SNIFFER_PRINT_TIMING
                snprintf(buf, sizeof(buf), "%02X/%d%c", (x & 0xFF), (x >> 16), (x & ACK ? ',' : '-'));

#else
                snprintf(buf, sizeof(buf), "%02X%c", static_cast<uint8_t>((x & 0xFF)), (x & ACK ? ',' : '-'));
#endif
                buffer += buf;
                if (buffer.length() == 4)
                { // start cond. + first hex byte
                    strlcpy(i2c_header_buf, buffer.c_str(), sizeof(i2c_header_buf));
                    if (i2c_header_buf[0] == '[' && i2c_header_buf[1] == '8' && i2c_header_buf[2] == '8' && ithoInitResult != -2)
                    {
                        log_entry_index = i2cLogger.i2cLogStart(I2C_CMD_TEMP_READ_ITHO);
                        i2c_sniffer_timer.once_ms(I2C_SAFE_GAURD_TIMER, i2cSafeGuardControl);
                    }
                }
            }
        }
    }
}

void i2cSafeGuardControl()
{
    auto *sg = static_cast<i2c_safe_guard_t *>(i2cManager.safe_guard);
    sg->update_time();
    if (sg->i2c_safe_guard_enabled == true && sg->sniffer_enabled == false)
    {
        i2cSnifferDisable();
        i2c_sniffer_timer.once_ms(7500, i2cSnifferEnable);
    }
}

void i2cSnifferProcessBuf(std::string &buffer)
{
    auto *sg = static_cast<i2c_safe_guard_t *>(i2cManager.safe_guard);
    if (sg->sniffer_enabled == true)
    {
        int len = buffer.length();
        if (len)
        {
            if (sg->sniffer_web_enabled)
            {
                JsonDocument root;
                root["i2csniffer"] = buffer.c_str();
                notifyClients(root.as<JsonObject>());
            }
            D_LOG(buffer.c_str());
        }
    }

    buffer = std::string();
}

void i2cSnifferSetPins(gpio_num_t sda, gpio_num_t scl)
{
    i2cManager.setSnifferPins(sda, scl);
}

void i2cSnifferInit(bool enabled)
{
    xTaskCreatePinnedToCore(sniffer_task, "sniffer_task", 4096, (void *)enabled, 17, NULL, I2C_SNIFFER_RUN_ON_CORE);
}

void i2cSnifferEnable()
{
    enable_sda_intr(true);
    auto *sg = static_cast<i2c_safe_guard_t *>(i2cManager.safe_guard);
    if (sg->i2c_safe_guard_enabled == true && sg->sniffer_enabled == true)
    {
        W_LOG("I2C: safe guard disabled due to enabled i2c sniffer");
    }
}

void i2cSnifferDisable()
{
    enable_sda_intr(false);
    vTaskDelay(1);
    enable_sda_intr(false);
}

void i2cSnifferUnload()
{
    i2c_sniffer_timer.detach();
    auto *sg = static_cast<i2c_safe_guard_t *>(i2cManager.safe_guard);
    sg->i2c_safe_guard_enabled = false;
    sg->sniffer_enabled = false;
    i2cSnifferDisable();
}

void i2cSnifferPullup(bool enable)
{
    if (enable)
    {
        gpio_pullup_en(i2cManager.sniffer_scl_pin);
        gpio_pullup_en(i2cManager.sniffer_sda_pin);
    }
    else
    {
        gpio_pullup_dis(i2cManager.sniffer_scl_pin);
        gpio_pullup_dis(i2cManager.sniffer_sda_pin);
    }
}