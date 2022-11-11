#include "i2c_esp32.h"

char toHex(uint8_t c)
{
  return c < 10 ? c + '0' : c + 'A' - 10;
}

uint8_t i2cbuf[I2C_SLAVE_RX_BUF_LEN];
static size_t buflen;

esp_err_t i2c_master_init(int log_entry_idx)
{

  esp_err_t result;

  if (!checkI2Cbus(log_entry_idx))
    result = ESP_FAIL;

#ifdef ESPRESSIF32_3_5_0
  i2c_config_t conf = {I2C_MODE_MASTER, I2C_MASTER_SDA_IO, I2C_MASTER_SDA_PULLUP, I2C_MASTER_SCL_IO, I2C_MASTER_SCL_PULLUP, {.master = {I2C_MASTER_FREQ_HZ}}};
#else
  i2c_config_t conf = {
      .mode = I2C_MODE_MASTER,
      .sda_io_num = I2C_MASTER_SDA_IO,
      .scl_io_num = I2C_MASTER_SCL_IO,
      .sda_pullup_en = I2C_MASTER_SDA_PULLUP,
      .scl_pullup_en = I2C_MASTER_SCL_PULLUP,
      .master = {
          .clk_speed = I2C_MASTER_FREQ_HZ,
      },
  };
#endif

  i2c_param_config(I2C_MASTER_NUM, &conf);

  result = i2c_driver_install(I2C_MASTER_NUM, conf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
  if (result != ESP_OK)
  {
    D_LOG("i2c_master_init error: %s\n", esp_err_to_name(result));
  }

  return result;
}

void i2c_master_deinit()
{
  i2c_driver_delete(I2C_MASTER_NUM);
}

esp_err_t i2c_master_send(const char *buf, uint32_t len, int log_entry_idx)
{
  esp_err_t rc = i2c_master_init(log_entry_idx);

  if (rc != ESP_OK)
    return rc;

  i2c_set_timeout(I2C_MASTER_NUM, 0xFFFFF);

  i2c_cmd_handle_t link = i2c_cmd_link_create();
  i2c_master_start(link);
  i2c_master_write(link, (uint8_t *)buf, len, true);
  i2c_master_stop(link);
  rc = i2c_master_cmd_begin(I2C_MASTER_NUM, link, 200 / portTICK_RATE_MS);
  i2c_cmd_link_delete(link);

  i2c_master_deinit();

  return rc;
}

esp_err_t i2c_master_send_command(uint8_t addr, const uint8_t *cmd, uint32_t len, int log_entry_idx)
{

  esp_err_t rc = i2c_master_init(log_entry_idx);

  if (rc != ESP_OK)
    return rc;

  i2c_set_timeout(I2C_MASTER_NUM, 0xFFFFF);

  i2c_cmd_handle_t link = i2c_cmd_link_create();
  i2c_master_start(link);
  i2c_master_write_byte(link, (addr << 1) | WRITE_BIT, ACK_CHECK_EN);
  i2c_master_write(link, (uint8_t *)cmd, len, true);
  i2c_master_stop(link);
  rc = i2c_master_cmd_begin(I2C_MASTER_NUM, link, 200 / portTICK_RATE_MS);
  i2c_cmd_link_delete(link);

  i2c_master_deinit();

  return rc;
}

esp_err_t i2c_master_read_slave(uint8_t addr, uint8_t *data_rd, size_t size, I2CLogger::i2c_cmdref_t origin)
{

  if (ithoInitResult == -2)
  {
    return ESP_FAIL;
  }

  int log_entry_idx = i2cLogger.i2c_log_start(origin);

  esp_err_t rc = i2c_master_init(log_entry_idx);

  if (rc != ESP_OK)
  {
    i2cLogger.i2c_log_final(log_entry_idx, I2CLogger::I2C_ERROR_MASTER_INIT_FAIL);
    return rc;
  }

  i2c_set_timeout(I2C_MASTER_NUM, 0xFFFFF);

  i2c_cmd_handle_t link = i2c_cmd_link_create();
  i2c_master_start(link);
  i2c_master_write_byte(link, (addr << 1) | READ_BIT, ACK_CHECK_EN);
  if (size > 1)
  {
    i2c_master_read(link, data_rd, size - 1, (i2c_ack_type_t)ACK_VAL);
  }
  i2c_master_read_byte(link, data_rd + size - 1, (i2c_ack_type_t)NACK_VAL);
  i2c_master_stop(link);
  rc = i2c_master_cmd_begin(I2C_MASTER_NUM, link, 2000);
  i2c_cmd_link_delete(link);

  i2c_master_deinit();

  i2cLogger.i2c_log_final(log_entry_idx, I2CLogger::I2C_OK);
  return rc;
}

esp_err_t i2c_master_read_slave(uint8_t addr, uint8_t *data_rd, size_t size)
{
  return i2c_master_read_slave(addr, data_rd, size, I2CLogger::I2C_CMD_UNKOWN);
}

bool i2c_sendBytes(const uint8_t *buf, size_t len, I2CLogger::i2c_cmdref_t origin)
{
  if (ithoInitResult == -2)
  {
    return false;
  }

  int log_entry_idx = i2cLogger.i2c_log_start(origin);

  if (len)
  {
    esp_err_t rc = i2c_master_send((const char *)buf, len, log_entry_idx);
    if (rc)
    {
      // D_LOG("Master send: %d\n", rc);
      i2cLogger.i2c_log_final(log_entry_idx, I2CLogger::I2C_NOK);
      return false;
    }
    i2cLogger.i2c_log_final(log_entry_idx, I2CLogger::I2C_OK);
    return true;
  }
  i2cLogger.i2c_log_final(log_entry_idx, I2CLogger::I2C_ERROR_LEN);
  return false;
}

bool i2c_sendBytes(const uint8_t *buf, size_t len)
{
  return i2c_sendBytes(buf, len, I2CLogger::I2C_CMD_UNKOWN);
}

bool i2c_sendCmd(uint8_t addr, const uint8_t *cmd, size_t len, I2CLogger::i2c_cmdref_t origin)
{
  if (ithoInitResult == -2)
  {
    return false;
  }

  int log_entry_idx = i2cLogger.i2c_log_start(origin);

  if (len)
  {
    esp_err_t rc = i2c_master_send_command(addr, cmd, len, log_entry_idx);
    if (rc)
    {
      // D_LOG("Master send: %d\n", rc);
      i2cLogger.i2c_log_final(log_entry_idx, I2CLogger::I2C_NOK);
      return false;
    }
    i2cLogger.i2c_log_final(log_entry_idx, I2CLogger::I2C_OK);
    return true;
  }
  i2cLogger.i2c_log_final(log_entry_idx, I2CLogger::I2C_ERROR_LEN);
  return false;
}

bool i2c_sendCmd(uint8_t addr, const uint8_t *cmd, size_t len)
{
  return i2c_sendCmd(addr, cmd, len, I2CLogger::I2C_CMD_UNKOWN);
}

size_t i2c_slave_receive(uint8_t i2c_receive_buf[])
{

  // typedef struct
  // {
  //   i2c_mode_t mode;    /*!< I2C mode */
  //   int sda_io_num;     /*!< GPIO number for I2C sda signal */
  //   int scl_io_num;     /*!< GPIO number for I2C scl signal */
  //   bool sda_pullup_en; /*!< Internal GPIO pull mode for I2C sda signal*/
  //   bool scl_pullup_en; /*!< Internal GPIO pull mode for I2C scl signal*/

  //   union
  //   {
  //     struct
  //     {
  //       uint32_t clk_speed; /*!< I2C clock frequency for master mode, (no higher than 1MHz for now) */
  //     } master;             /*!< I2C master config */
  //     struct
  //     {
  //       uint8_t addr_10bit_en;  /*!< I2C 10bit address mode enable for slave mode */
  //       uint16_t slave_addr;    /*!< I2C address for slave mode */
  //       uint32_t maximum_speed; /*!< I2C expected clock speed from SCL. */
  //     } slave;                  /*!< I2C slave config */
  //   };
  //   uint32_t clk_flags; /*!< Bitwise of ``I2C_SCLK_SRC_FLAG_**FOR_DFS**`` for clk source choice*/
  // } i2c_config_t;

#ifdef ESPRESSIF32_3_5_0
  i2c_config_t conf_slave = {I2C_MODE_SLAVE, I2C_SLAVE_SDA_IO, I2C_SLAVE_SDA_PULLUP, I2C_SLAVE_SCL_IO, I2C_SLAVE_SCL_PULLUP, {.slave = {0, I2C_SLAVE_ADDRESS}}};
#else
  i2c_config_t conf_slave = {
      .mode = I2C_MODE_SLAVE,
      .sda_io_num = I2C_SLAVE_SDA_IO,
      .scl_io_num = I2C_SLAVE_SCL_IO,
      .sda_pullup_en = I2C_SLAVE_SDA_PULLUP,
      .scl_pullup_en = I2C_SLAVE_SCL_PULLUP,
      .slave = {
          .addr_10bit_en = 0,
          .slave_addr = I2C_SLAVE_ADDRESS,
      }, // address of your project
  };
#endif

  i2c_param_config(I2C_SLAVE_NUM, &conf_slave);

  i2c_driver_install(I2C_SLAVE_NUM, conf_slave.mode, I2C_SLAVE_RX_BUF_LEN, I2C_SLAVE_TX_BUF_LEN, 0);

  i2c_set_timeout(I2C_SLAVE_NUM, 0xFFFFF);

  i2cbuf[0] = I2C_SLAVE_ADDRESS << 1;
  buflen = 1;

  while (1)
  {
    int len1 = i2c_slave_read_buffer(I2C_SLAVE_NUM, i2cbuf + buflen, sizeof(i2cbuf) - buflen, 50 / portTICK_RATE_MS);
    if (len1 <= 0)
      break;
    buflen += len1;
  }
  if (buflen > 1)
  {
    for (uint16_t i = 0; i < buflen; i++)
    {
      i2c_receive_buf[i] = i2cbuf[i];
    }
  }
  i2c_slave_deinit();
  return buflen;
}

void i2c_slave_deinit()
{
  i2c_driver_delete(I2C_SLAVE_NUM);
}

bool checkI2Cbus(int log_entry_idx)
{

  bool scl_high = digitalRead(I2C_MASTER_SCL_IO) == HIGH;
  bool sda_high = digitalRead(I2C_MASTER_SDA_IO) == HIGH;

  if (scl_high && sda_high)
  {
    return true;
  }
  // else
  // {
  //   if (!scl_high && !sda_high)
  //   {
  //     i2cLogger.i2c_log_err_state(log_entry_idx, I2CLogger::I2C_ERROR_SCL_SDA_LOW);
  //   }
  //   else if (!scl_high)
  //   {
  //     i2cLogger.i2c_log_err_state(log_entry_idx, I2CLogger::I2C_ERROR_SCL_LOW);
  //   }
  //   else
  //   {
  //     i2cLogger.i2c_log_err_state(log_entry_idx, I2CLogger::I2C_ERROR_SDA_LOW);
  //   }

  // }

  // read I2C pin state as fast as possible and return if both are HIGH for 100uS (about 10 I2C clock periods at 100kHz)
  // Using GPIO.in directly seems to be 3x faster compared to DigitalRead
  // source: https://www.reddit.com/r/esp32/comments/f529hf/results_comparing_the_speeds_of_different_gpio/
  // As I2C_MASTER_SCL_IO and I2C_MASTER_SDA_IO are within the range GPIO0~31, GPIO.in can be used
  unsigned long pin_check_timout = micros();
  while (digitalRead(I2C_MASTER_SCL_IO) == HIGH && digitalRead(I2C_MASTER_SDA_IO) == HIGH)
  {
    if (micros() - pin_check_timout < 100)
      return true;
  }

  unsigned int timeout = 200;
  unsigned int startread = millis();
  // unsigned int startbusy = millis();

  unsigned int cntr = 0;
  // bool log_i2cbus_busy = false;

  while ((digitalRead(I2C_MASTER_SCL_IO) == LOW || digitalRead(I2C_MASTER_SDA_IO) == LOW) && cntr < 10)
  {
    // log_i2cbus_busy = true;

    if (millis() - startread > timeout)
    {
      cntr++;
      startread = millis();
    }
    yield();
    delay(1);
  }
  if (cntr > 9)
  {
    // logInput("Warning: I2C timeout, trying I2C bus reset...");
    int result = I2C_ClearBus();
    if (result != 0)
    {
      if (result == 1)
      {
        i2cLogger.i2c_log_err_state(log_entry_idx, I2CLogger::I2C_ERROR_SCL_LOW);
      }
      else if (result == 2)
      {
        i2cLogger.i2c_log_err_state(log_entry_idx, I2CLogger::I2C_ERROR_SLAVE_CLOCKSTRETCH);
      }
      else if (result == 3)
      {
        i2cLogger.i2c_log_err_state(log_entry_idx, I2CLogger::I2C_ERROR_SDA_LOW);
      }
      logInput("Error: I2C bus could not be cleared!");
      ithoInitResult = -2; // stop I2C
    }
    else
    {
      i2cLogger.i2c_log_err_state(log_entry_idx, I2CLogger::I2C_ERROR_CLEARED_BUS);
      return true;
    }
    return false;
  }
  else
  {
    // if (log_i2cbus_busy)
    // {
    //   char buf[64]{};
    //   snprintf(buf, sizeof(buf), "Info: I2C bus busy, cleared after %dms", millis() - startbusy);
    //   logInput(buf);
    // }
    i2cLogger.i2c_log_err_state(log_entry_idx, I2CLogger::I2C_ERROR_CLEARED_OK);
    return true;
  }
}

/**
 *
 * Credits for this function: http://www.forward.com.au/pfod/ArduinoProgramming/I2C_ClearBus/index.html
 *
 * This routine turns off the I2C bus and clears it
 * on return SCA and SCL pins are tri-state inputs.
 * You need to call Wire.begin() after this to re-enable I2C
 * This routine does NOT use the Wire library at all.
 *
 * returns 0 if bus cleared
 *         1 if SCL held low.
 *         2 if SDA held low by slave clock stretch for > 2sec
 *         3 if SDA held low after 20 clocks.
 */
int I2C_ClearBus()
{
  // Remove any existing I2C drivers
  i2c_master_deinit();
  i2c_slave_deinit();

  pinMode(I2C_MASTER_SDA_IO, INPUT_PULLUP); // Make SDA (data) and SCL (clock) pins Inputs with pullup.
  pinMode(I2C_MASTER_SCL_IO, INPUT_PULLUP);

  delay(2500); // Wait 2.5 secs. This is strictly only necessary on the first power
  // up of the DS3231 module to allow it to initialize properly,
  // but is also assists in reliable programming of FioV3 boards as it gives the
  // IDE a chance to start uploaded the program
  // before existing sketch confuses the IDE by sending Serial data.

  boolean SCL_LOW = (digitalRead(I2C_MASTER_SCL_IO) == LOW); // Check is SCL is Low.
  if (SCL_LOW)
  {           // If it is held low Arduno cannot become the I2C master.
    return 1; // I2C bus error. Could not clear SCL clock line held low
  }

  boolean SDA_LOW = (digitalRead(I2C_MASTER_SDA_IO) == LOW); // vi. Check SDA input.
  int clockCount = 20;                                       // > 2x9 clock

  while (SDA_LOW && (clockCount > 0))
  { //  vii. If SDA is Low,
    clockCount--;
    // Note: I2C bus is open collector so do NOT drive SCL or SDA high.
    pinMode(I2C_MASTER_SCL_IO, INPUT);        // release SCL pullup so that when made output it will be LOW
    pinMode(I2C_MASTER_SCL_IO, OUTPUT);       // then clock SCL Low
    delayMicroseconds(10);                    //  for >5us
    pinMode(I2C_MASTER_SCL_IO, INPUT);        // release SCL LOW
    pinMode(I2C_MASTER_SCL_IO, INPUT_PULLUP); // turn on pullup resistors again
    // do not force high as slave may be holding it low for clock stretching.
    delayMicroseconds(10); //  for >5us
    // The >5us is so that even the slowest I2C devices are handled.
    SCL_LOW = (digitalRead(I2C_MASTER_SCL_IO) == LOW); // Check if SCL is Low.
    int counter = 20;
    while (SCL_LOW && (counter > 0))
    { //  loop waiting for SCL to become High only wait 2sec.
      counter--;
      delay(100);
      SCL_LOW = (digitalRead(I2C_MASTER_SCL_IO) == LOW);
    }
    if (SCL_LOW)
    {           // still low after 2 sec error
      return 2; // I2C bus error. Could not clear. SCL clock line held low by slave clock stretch for >2sec
    }
    SDA_LOW = (digitalRead(I2C_MASTER_SDA_IO) == LOW); //   and check SDA input again and loop
  }
  if (SDA_LOW)
  {           // still low
    return 3; // I2C bus error. Could not clear. SDA data line held low
  }

  // else pull SDA line low for Start or Repeated Start
  pinMode(I2C_MASTER_SDA_IO, INPUT);  // remove pullup.
  pinMode(I2C_MASTER_SDA_IO, OUTPUT); // and then make it LOW i.e. send an I2C Start or Repeated start control.
  // When there is only one I2C master a Start or Repeat Start has the same function as a Stop and clears the bus.
  /// A Repeat Start is a Start occurring after a Start with no intervening Stop.
  delayMicroseconds(10);                    // wait >5us
  pinMode(I2C_MASTER_SDA_IO, INPUT);        // remove output low
  pinMode(I2C_MASTER_SDA_IO, INPUT_PULLUP); // and make SDA high i.e. send I2C STOP control.
  delayMicroseconds(10);                    // x. wait >5us
  pinMode(I2C_MASTER_SDA_IO, INPUT);        // and reset pins as tri-state inputs which is the default state on reset
  pinMode(I2C_MASTER_SCL_IO, INPUT);
  return 0; // all ok
}

void trigger_sht_sensor_reset()
{

  /*
  from: Sensirion_Humidity_Sensors_SHT3x_Datasheet_digital.pdf
  Interface Reset
    If communication with the device is lost, the following
    signal sequence will reset the serial interface: While
    leaving SDA high, toggle SCL nine or more times. This
    must be followed by a Transmission Start sequence
    preceding the next command. This sequence resets the
    interface only. The status register preserves its content.
  */
  // Remove any existing I2C drivers
  i2c_master_deinit();
  i2c_slave_deinit();

  pinMode(I2C_MASTER_SDA_IO, OUTPUT); // Make SDA (data) and SCL (clock) pins outputs
  pinMode(I2C_MASTER_SCL_IO, OUTPUT);

  digitalWrite(I2C_MASTER_SDA_IO, HIGH);
  digitalWrite(I2C_MASTER_SCL_IO, HIGH);

  for (uint i = 0; i < 10; i++)
  {
    digitalWrite(I2C_MASTER_SCL_IO, LOW);
    delayMicroseconds(10);
    digitalWrite(I2C_MASTER_SCL_IO, HIGH);
    delayMicroseconds(10);
  }

  pinMode(I2C_MASTER_SDA_IO, INPUT); // and reset pins as tri-state inputs which is the default state on reset
  pinMode(I2C_MASTER_SCL_IO, INPUT);

  delayMicroseconds(20);
}