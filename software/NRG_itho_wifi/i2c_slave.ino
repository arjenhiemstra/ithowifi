
#include <driver/i2c.h>
#include <esp_log.h>
#include <esp_system.h>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include <string>
#include <driver/gpio.h>
#include "hardware.h"

//#define I2C_SLAVE_SDA_IO     GPIO_NUM_21
//#define I2C_SLAVE_SCL_IO     GPIO_NUM_22
//#define I2C_SLAVE_SDA_PULLUP GPIO_PULLUP_DISABLE
//#define I2C_SLAVE_SCL_PULLUP GPIO_PULLUP_DISABLE
//#define I2C_SLAVE_NUM        I2C_NUM_0
//#define I2C_SLAVE_RX_BUF_LEN 512
//#define I2C_SLAVE_ADDRESS    0x40
//
//#define I2C_MASTER_SDA_IO     GPIO_NUM_21
//#define I2C_MASTER_SCL_IO     GPIO_NUM_22
//#define I2C_MASTER_SDA_PULLUP GPIO_PULLUP_DISABLE
//#define I2C_MASTER_SCL_PULLUP GPIO_PULLUP_DISABLE
//#define I2C_MASTER_NUM        I2C_NUM_1
//#define I2C_MASTER_FREQ_HZ    100000
//
//#define WRITE_BIT I2C_MASTER_WRITE /*!< I2C master write */
//#define READ_BIT I2C_MASTER_READ   /*!< I2C master read */
//#define ACK_CHECK_EN 0x1           /*!< I2C master will check ack from slave*/
//#define ACK_CHECK_DIS 0x0 /*!< I2C master will not check ack from slave */
//#define ACK_VAL 0x0       /*!< I2C ack value */
//#define NACK_VAL 0x1      /*!< I2C nack value */

#define I2C_MASTER_TX_BUF_DISABLE 0 /*!< I2C master doesn't need buffer */
#define I2C_MASTER_RX_BUF_DISABLE 0 /*!< I2C master doesn't need buffer */
#define WRITE_BIT I2C_MASTER_WRITE  /*!< I2C master write */
#define READ_BIT I2C_MASTER_READ    /*!< I2C master read */
#define ACK_CHECK_EN 0x1            /*!< I2C master will check ack from slave*/
#define ACK_CHECK_DIS 0x0           /*!< I2C master will not check ack from slave */
#define ACK_VAL 0x0                 /*!< I2C ack value */
#define NACK_VAL 0x1                /*!< I2C nack value */

static const char *TAG = "cmd_i2ctools";

static gpio_num_t i2c_gpio_sda = (gpio_num_t)21;
static gpio_num_t i2c_gpio_scl = (gpio_num_t)22;
static uint32_t i2c_frequency = 100000;
static i2c_port_t i2c_port = I2C_NUM_0;


inline uint8_t checksum(const uint8_t* buf, size_t buflen) {
  uint8_t sum = 0;
  while (buflen--) {
    sum += *buf++;
  }
  return -sum;
}

inline char toHex(uint8_t c) {
  return c < 10 ? c + '0' : c + 'A' - 10;
}
inline bool checksumOk(const uint8_t* data, size_t len) {
  return checksum(data, len - 1) == data[len - 1];
}

static esp_err_t i2c_master_driver_initialize()
{
    i2c_config_t conf = {
        conf.mode = I2C_MODE_MASTER,
        conf.sda_io_num = i2c_gpio_sda,
        conf.sda_pullup_en = GPIO_PULLUP_ENABLE,
        conf.scl_io_num = i2c_gpio_scl,
        conf.scl_pullup_en = GPIO_PULLUP_ENABLE,
        conf.master.clk_speed = i2c_frequency
    };
    return i2c_param_config(i2c_port, &conf);
}
//static esp_err_t i2c_master_send(char* buf, uint32_t len) {
//  i2c_cmd_handle_t cmd = i2c_cmd_link_create();
//  i2c_master_start(cmd);
//  //i2c_master_write_byte(cmd, (ESP_SLAVE_ADDR << 1) | WRITE_BIT, ACK_CHECK_EN);
//  i2c_master_write(cmd, (uint8_t*)buf, len, ACK_CHECK_EN);
//  i2c_master_stop(cmd);
//  esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_RATE_MS);
//  i2c_cmd_link_delete(cmd);
//  return ret;
//}
String i2c_master_send(char* buf, size_t len)
{
    String s;
    i2c_master_driver_initialize();
    i2c_driver_install(i2c_port, I2C_MODE_MASTER, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
//    i2c_master_write_byte(cmd, chip_addr << 1 | WRITE_BIT, ACK_CHECK_EN);
//    if (i2cset_args.register_address->count) {
//        i2c_master_write_byte(cmd, data_addr, ACK_CHECK_EN);
//    }
    for (int i = 0; i < len; i++) {
        i2c_master_write_byte(cmd, buf[i], ACK_CHECK_EN);
    }
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(i2c_port, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
    if (ret == ESP_OK) {
        s = "Write OK";
    } else if (ret == ESP_ERR_TIMEOUT) {
        s = "Bus is busy";
    } else {
        s = "Write Failed";
    }
    i2c_driver_delete(i2c_port);
    return s;
}



//portMUX_TYPE status_mux = portMUX_INITIALIZER_UNLOCKED;
//std::string datatypes;
//
//static const char* TAG = "i2c-slave";
//
//static i2c_slave_callback_t i2c_callback;
//static uint8_t buf[I2C_SLAVE_RX_BUF_LEN];
//static size_t buflen;
//
//
//
//static void i2c_slave_task(void* arg) {
//  while (1) {
//    buf[0] = I2C_SLAVE_ADDRESS << 1;
//    buflen = 1;
//    while (1) {
//      int len1 = i2c_slave_read_buffer(I2C_SLAVE_NUM, buf + buflen, sizeof(buf) - buflen, 10);
//      if (len1 <= 0)
//        break;
//      buflen += len1;
//    }
//    if (buflen > 1) {
//      i2c_callback(buf, buflen);
//      break;
//    }
//  }
//  vTaskDelete( NULL );
//}
//
//void i2c_slave_init(i2c_slave_callback_t cb) {
//  i2c_config_t conf = {I2C_MODE_SLAVE,   I2C_SLAVE_SDA_IO,     I2C_SLAVE_SDA_PULLUP,
//                       I2C_SLAVE_SCL_IO, I2C_SLAVE_SCL_PULLUP, {.slave = {0, I2C_SLAVE_ADDRESS}}
//                      };
//  i2c_param_config(I2C_SLAVE_NUM, &conf);
//  i2c_callback = cb;
//  i2c_driver_install(I2C_SLAVE_NUM, conf.mode, I2C_SLAVE_RX_BUF_LEN, 0, 0);
//  //    ESP_LOGI(TAG, "Slave pin assignment: SCL=%d, SDA=%d", I2C_SLAVE_SCL_IO, I2C_SLAVE_SDA_IO);
//  //    ESP_LOGI(TAG, "Slave address: %02X (W)", I2C_SLAVE_ADDRESS << 1);
//  xTaskCreatePinnedToCore(i2c_slave_task, "i2c_task", 4096, NULL, 22, NULL, 0);
//}
//
//void i2c_slave_deinit() {
//  esp_err_t result = i2c_driver_delete(I2C_SLAVE_NUM);
//}
//
//void i2c_slave_callback(const uint8_t* data, size_t len) {
//  std::string s;
//  s.reserve(len * 3 + 2);
//  for (size_t i = 0; i < len; ++i) {
//    if (i)
//      s += ' ';
//    s += toHex(data[i] >> 4);
//    s += toHex(data[i] & 0xF);
//  }
//  if (len > 6 && data[1] == 0x82 && data[2] == 0xA4 && data[3] == 0 && data[4] == 1 && data[5] < len - 5 && checksumOk(data, len)) {
//    portENTER_CRITICAL(&status_mux);
//    datatypes.assign((const char*)data + 6, data[5]);
//    portEXIT_CRITICAL(&status_mux);
//  }
//  if (len > 6 && data[1] == 0x82 && data[2] == 0xa4 && data[3] == 1 && data[4] == 1 && data[5] < len - 5 && checksumOk(data, len)) {
//    //handleStatus(data + 6, len - 7);
//  }
//
//  sprintf(i2cresult, "Slave: %s\n", s.c_str());
//  callback_called = true;
//
//}
//
//
//
//
//static esp_err_t i2c_master_init() {
//  i2c_port_t i2c_master_port = I2C_MASTER_NUM;
//  i2c_config_t conf;
//  conf.mode = I2C_MODE_MASTER;
//  conf.sda_io_num = I2C_MASTER_SDA_IO;
//  conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
//  conf.scl_io_num = I2C_MASTER_SCL_IO;
//  conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
//  conf.master.clk_speed = I2C_MASTER_FREQ_HZ;
//  i2c_param_config(i2c_master_port, &conf);
//  return i2c_driver_install(i2c_master_port, conf.mode,0,0,0);
//}
//
//void i2c_master_deinit() {
//  esp_err_t result = i2c_driver_delete(I2C_MASTER_NUM);
//}
//
//
//static esp_err_t i2c_master_send(char* buf, uint32_t len) {
//  i2c_cmd_handle_t cmd = i2c_cmd_link_create();
//  i2c_master_start(cmd);
//  //i2c_master_write_byte(cmd, (ESP_SLAVE_ADDR << 1) | WRITE_BIT, ACK_CHECK_EN);
//  i2c_master_write(cmd, (uint8_t*)buf, len, ACK_CHECK_EN);
//  i2c_master_stop(cmd);
//  esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_RATE_MS);
//  i2c_cmd_link_delete(cmd);
//  return ret;
//}

//
//
//esp_err_t i2c_master_send(char* buf, uint32_t len) {
//    esp_err_t rc;
////    for (uint32_t i = 0; i < 5; ++i) {
//        i2c_cmd_handle_t link = i2c_cmd_link_create();
//        i2c_master_start(link);
//        i2c_master_write(link, (uint8_t*)buf, len, true);
//        i2c_master_stop(link);
//        rc = i2c_master_cmd_begin(I2C_MASTER_NUM, link, 25);
//        i2c_cmd_link_delete(link);
////        if (!rc)
////            break;
//        vTaskDelay(1);
////    }
//    return rc;
//}
//
//bool sendBytes(const uint8_t* buf, size_t len) {
//    if (len) {
//        esp_err_t rc = i2c_master_send((char*)buf, len);
//        if (rc) {
//            printf("Master send: %d\n", rc);
//        }
//        return rc == ESP_OK;
//    }
//    return false;
//}
