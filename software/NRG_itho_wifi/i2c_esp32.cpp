#include <string>

#include <Arduino.h>
#include <driver/i2c.h>
#include <esp_log.h>
#include <esp_system.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>

#include "i2c_esp32.h"
#include "IthoSystem.h"


char toHex(uint8_t c) {
  return c < 10 ? c + '0' : c + 'A' - 10;
}

//static i2c_slave_callback_t i2c_callback;
uint8_t i2cbuf[I2C_SLAVE_RX_BUF_LEN];
static size_t buflen;

void i2c_master_init() {
  i2c_config_t conf = {I2C_MODE_MASTER,   I2C_MASTER_SDA_IO,     I2C_MASTER_SDA_PULLUP,
                       I2C_MASTER_SCL_IO, I2C_MASTER_SCL_PULLUP, {.master = {I2C_MASTER_FREQ_HZ}}
                      };
  i2c_param_config(I2C_MASTER_NUM, &conf);
  i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0);
}

void i2c_master_deinit() {
  i2c_driver_delete(I2C_MASTER_NUM);
}

esp_err_t i2c_master_send(const char* buf, uint32_t len) {
  i2c_master_init();

  esp_err_t rc;
  i2c_set_timeout(I2C_MASTER_NUM, 0xFFFFF);

  i2c_cmd_handle_t link = i2c_cmd_link_create();
  i2c_master_start(link);
  i2c_master_write(link, (uint8_t*)buf, len, true);
  i2c_master_stop(link);
  rc = i2c_master_cmd_begin(I2C_MASTER_NUM, link, 200 / portTICK_RATE_MS);
  i2c_cmd_link_delete(link);

  i2c_master_deinit();

  return rc;
}

esp_err_t i2c_master_send_command(uint8_t addr, const uint8_t* cmd, uint32_t len) {

  if (xSemaphoreTake(mutexI2Ctask, (TickType_t) 500 / portTICK_PERIOD_MS) == pdTRUE) {
    i2c_master_init();

    esp_err_t rc;
    i2c_set_timeout(I2C_MASTER_NUM, 0xFFFFF);

    i2c_cmd_handle_t link = i2c_cmd_link_create();
    i2c_master_start(link);
    i2c_master_write_byte(link, (addr << 1) | WRITE_BIT, ACK_CHECK_EN);
    i2c_master_write(link, (uint8_t*)cmd, len, true);
    i2c_master_stop(link);
    rc = i2c_master_cmd_begin(I2C_MASTER_NUM, link, 200 / portTICK_RATE_MS);
    i2c_cmd_link_delete(link);

    i2c_master_deinit();

    xSemaphoreGive(mutexI2Ctask);

    return rc;
  }
  else {
    return -1;
  }


}

esp_err_t i2c_master_read_slave(uint8_t addr, uint8_t *data_rd, size_t size) {

  if (xSemaphoreTake(mutexI2Ctask, (TickType_t) 500 / portTICK_PERIOD_MS) == pdTRUE) {
    i2c_master_init();

    esp_err_t rc;
    i2c_set_timeout(I2C_MASTER_NUM, 0xFFFFF);

    i2c_cmd_handle_t link = i2c_cmd_link_create();
    i2c_master_start(link);
    i2c_master_write_byte(link, (addr << 1) | READ_BIT, ACK_CHECK_EN);
    if (size > 1) {
      i2c_master_read(link, data_rd, size - 1, (i2c_ack_type_t)ACK_VAL);
    }
    i2c_master_read_byte(link, data_rd + size - 1, (i2c_ack_type_t)NACK_VAL);
    i2c_master_stop(link);
    rc = i2c_master_cmd_begin(I2C_MASTER_NUM, link, 2000);
    i2c_cmd_link_delete(link);

    i2c_master_deinit();

    xSemaphoreGive(mutexI2Ctask);

    return rc;
  }
  else {
    return -1;
  }


}

bool i2c_sendBytes(const uint8_t* buf, size_t len) {

  if (len) {
    esp_err_t rc = i2c_master_send((const char*)buf, len);
    if (rc) {
      D_LOG("Master send: %d\n", rc);
      return false;
    }
    return true;
  }
  return false;
}

bool i2c_sendCmd(uint8_t addr, const uint8_t* cmd, size_t len) {
  if (len) {
    esp_err_t rc = i2c_master_send_command(addr, cmd, len);
    if (rc) {
      D_LOG("Master send: %d\n", rc);
      return false;
    }
    return true;
  }
  return false;
}

//void i2c_slave_callback(const uint8_t* data, size_t len) {
//    for(uint16_t i=0;i<I2C_SLAVE_RX_BUF_LEN;i++) {
//      i2c_slave_data[i] = 0;
//    }
//    for(uint16_t i=0;i<len;i++) {
//      i2c_slave_data[i] = data[i];
//    }
//    std::string s;
//    s.reserve(len * 3 + 2);
//    for (size_t i = 0; i < len; ++i) {
//      if (i)
//        s += ' ';
//      s += toHex(data[i] >> 4);
//      s += toHex(data[i] & 0xF);
//    }
//    strcpy(i2c_slave_buf, "");
//    strlcpy(i2c_slave_buf, s.c_str(), sizeof(i2c_slave_buf));
//
//  //    if (len > 6 && data[1] == 0x82 && data[2] == 0xA4 && data[3] == 0 && data[4] == 1 && data[5] < len - 5 && checksumOk(data, len)) {
//  //        portENTER_CRITICAL(&status_mux);
//  //        datatypes.assign((const char*)data + 6, data[5]);
//  //        portEXIT_CRITICAL(&status_mux);
//  //    }
//  //    if (len > 6 && data[1] == 0x82 && data[2] == 0xa4 && data[3] == 1 && data[4] == 1 && data[5] < len - 5 && checksumOk(data, len)) {
//  //        handleStatus(data + 6, len - 7);
//  //    }
//  //    if (reportHex) {
//  //        mqtt_publish_bin("esp-data-hex", s.c_str(), s.length());
//  //    }
//  //    if (verbose) {
//  //        D_LOG("Slave: %s\n", s.c_str());
//  //    }
//
//  callback_called = true;
//
//}



//static void i2c_slave_task(void* arg) {
//  while (1) {
//    i2cbuf[0] = I2C_SLAVE_ADDRESS << 1;
//    buflen = 1;
//    while (1) {
//      int len1 = i2c_slave_read_buffer(I2C_SLAVE_NUM, i2cbuf + buflen, sizeof(i2cbuf) - buflen, 10);
//      if (len1 <= 0)
//        break;
//      buflen += len1;
//    }
//    if (buflen > 1) {
//      i2c_callback(i2cbuf, buflen);
//      callback_called = true;
//      vTaskDelete( NULL );
//    }
//  }
//}

//void i2c_slave_init(i2c_slave_callback_t cb) {
//  i2c_config_t conf = {I2C_MODE_SLAVE,   I2C_SLAVE_SDA_IO,     I2C_SLAVE_SDA_PULLUP,
//                       I2C_SLAVE_SCL_IO, I2C_SLAVE_SCL_PULLUP, {.slave = {0, I2C_SLAVE_ADDRESS}}
//                      };
//  i2c_param_config(I2C_SLAVE_NUM, &conf);
//  i2c_callback = cb;
//  i2c_driver_install(I2C_SLAVE_NUM, conf.mode, I2C_SLAVE_RX_BUF_LEN, 0, 0);
//
//  i2cbuf[0] = I2C_SLAVE_ADDRESS << 1;
//  buflen = 1;
//
//  unsigned long timeoutmillis = millis() + 200;
//
//  while (millis() < timeoutmillis) {
//    while (1) {
//      int len1 = i2c_slave_read_buffer(I2C_SLAVE_NUM, i2cbuf + buflen, sizeof(i2cbuf) - buflen, 10);
//      if (len1 <= 0)
//        break;
//      buflen += len1;
//    }
//    if (buflen > 1) {
//      //callback_called = true;
//      i2c_callback(i2cbuf, buflen);
//    }
//  }
//
//
//  //xTaskCreatePinnedToCore(i2c_slave_task, "i2c_task", 4096, NULL, 22, &xTaskI2cSlaveHandle, 0);
//}

size_t i2c_slave_receive(uint8_t i2c_receive_buf[]) {
  i2c_config_t conf = {I2C_MODE_SLAVE,   I2C_SLAVE_SDA_IO,     I2C_SLAVE_SDA_PULLUP,
                       I2C_SLAVE_SCL_IO, I2C_SLAVE_SCL_PULLUP, {.slave = {0, I2C_SLAVE_ADDRESS}}
                      };
  i2c_param_config(I2C_SLAVE_NUM, &conf);

  i2c_driver_install(I2C_SLAVE_NUM, conf.mode, I2C_SLAVE_RX_BUF_LEN, 0, 0);

  i2c_set_timeout(I2C_SLAVE_NUM, 0xFFFFF);

  i2cbuf[0] = I2C_SLAVE_ADDRESS << 1;
  buflen = 1;

  while (1) {
    int len1 = i2c_slave_read_buffer(I2C_SLAVE_NUM, i2cbuf + buflen, sizeof(i2cbuf) - buflen, 50 / portTICK_RATE_MS );
    if (len1 <= 0)
      break;
    buflen += len1;
  }
  if (buflen > 1) {
    for (uint16_t i = 0; i < buflen; i++) {
      i2c_receive_buf[i] = i2cbuf[i];
    }
  }
  i2c_slave_deinit();
  return buflen;
}

//void i2c_slaveInit() {
//  i2c_slave_init(&i2c_slave_callback);
//}
//
void i2c_slave_deinit() {
  i2c_driver_delete(I2C_SLAVE_NUM);
}


//bool callback_called = false;
//char i2c_slave_buf[I2C_SLAVE_RX_BUF_LEN];
//uint8_t i2c_slave_data[I2C_SLAVE_RX_BUF_LEN];
