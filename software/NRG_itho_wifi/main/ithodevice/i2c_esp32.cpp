#include "i2c_esp32.h"

// I2C globals now managed by I2CManager (see managers/I2CManager.cpp)
// Removed: i2c_cmd_queue, master/slave pin globals

void i2cQueueAddCmd(const std::function<void()> func)
{
  if (i2cManager.cmd_queue.size() >= I2C_CMD_QUEUE_MAX_SIZE)
  {
    log_msg input;
    input.code = SYSLOG_WARNING;
    input.msg = "Warning: i2c_cmd_queue overflow";
    syslog_queue.push_back(input);
    return;
  }
  i2cManager.queueAdd(func);
}

void i2cMasterSetPins(gpio_num_t sda, gpio_num_t scl)
{
  i2cManager.setMasterPins(sda, scl);
}

void i2cSlaveSetPins(gpio_num_t sda, gpio_num_t scl)
{
  i2cManager.setSlavePins(sda, scl);
}

esp_err_t i2cMasterInit(int log_entry_idx)
{

  esp_err_t result;

  if (!checkI2Cbus(log_entry_idx))
    result = ESP_FAIL;

  i2c_config_t conf = {
      .mode = I2C_MODE_MASTER,
      .sda_io_num = i2cManager.master_sda_pin,
      .scl_io_num = i2cManager.master_scl_pin,
      .sda_pullup_en = I2C_MASTER_SDA_PULLUP,
      .scl_pullup_en = I2C_MASTER_SCL_PULLUP,
      .master = {
          .clk_speed = I2C_MASTER_FREQ_HZ,
      },
      .clk_flags = I2C_SCLK_SRC_FLAG_FOR_NOMAL, // optional
  };

  i2c_param_config(I2C_MASTER_NUM, &conf);

  result = i2c_driver_install(I2C_MASTER_NUM, conf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
  if (result != ESP_OK)
  {
    E_LOG("I2C: i2cMasterInit error: %s", esp_err_to_name(result));
  }

  return result;
}

void i2cMasterDeinit()
{
  i2c_driver_delete(I2C_MASTER_NUM);
}

esp_err_t i2cMasterSend(const char *buf, uint32_t len, int log_entry_idx)
{
  esp_err_t rc = i2cMasterInit(log_entry_idx);

  if (rc != ESP_OK)
    return rc;

  i2c_set_timeout(I2C_MASTER_NUM, 0xFFFFF);

  i2c_cmd_handle_t link = i2c_cmd_link_create();
  i2c_master_start(link);
  i2c_master_write(link, (uint8_t *)buf, len, true);
  i2c_master_stop(link);
  rc = i2c_master_cmd_begin(I2C_MASTER_NUM, link, 200 / portTICK_PERIOD_MS);
  i2c_cmd_link_delete(link);

  i2cMasterDeinit();

  return rc;
}

esp_err_t i2cMasterSendCommand(uint8_t addr, const uint8_t *cmd, uint32_t len, int log_entry_idx)
{

  esp_err_t rc = i2cMasterInit(log_entry_idx);

  if (rc != ESP_OK)
    return rc;

  i2c_set_timeout(I2C_MASTER_NUM, 0xFFFFF);

  i2c_cmd_handle_t link = i2c_cmd_link_create();
  i2c_master_start(link);
  i2c_master_write_byte(link, (addr << 1) | WRITE_BIT, ACK_CHECK_EN);
  i2c_master_write(link, (uint8_t *)cmd, len, true);
  i2c_master_stop(link);
  rc = i2c_master_cmd_begin(I2C_MASTER_NUM, link, 200 / portTICK_PERIOD_MS);

  i2c_cmd_link_delete(link);

  i2cMasterDeinit();

  return rc;
}

esp_err_t i2cMasterReadSlave(uint8_t addr, uint8_t *data_rd, size_t size, i2c_cmdref_t origin)
{

  if (ithoInitResult == -2)
  {
    return ESP_FAIL;
  }

  int log_entry_idx = i2cLogger.i2cLogStart(origin);

  esp_err_t rc = i2cMasterInit(log_entry_idx);

  if (rc != ESP_OK)
  {
    i2cLogger.i2cLogFinal(log_entry_idx, I2CLogger::I2C_ERROR_MASTER_INIT_FAIL);
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

  i2cMasterDeinit();

  i2cLogger.i2cLogFinal(log_entry_idx, I2CLogger::I2C_OK);
  return rc;
}

esp_err_t i2cMasterReadSlave(uint8_t addr, uint8_t *data_rd, size_t size)
{
  return i2cMasterReadSlave(addr, data_rd, size, I2C_CMD_UNKOWN);
}

bool i2cSendBytes(const uint8_t *buf, size_t len, i2c_cmdref_t origin)
{
  if (ithoInitResult == -2)
  {
    return false;
  }

  int log_entry_idx = i2cLogger.i2cLogStart(origin);

  if (len)
  {
    esp_err_t rc = i2cMasterSend((const char *)buf, len, log_entry_idx);
    if (rc)
    {
      // D_LOG("Master send: %d", rc);
      i2cLogger.i2cLogFinal(log_entry_idx, I2CLogger::I2C_NOK);
      return false;
    }
    i2cLogger.i2cLogFinal(log_entry_idx, I2CLogger::I2C_OK);
    return true;
  }
  i2cLogger.i2cLogFinal(log_entry_idx, I2CLogger::I2C_ERROR_LEN);
  return false;
}

bool i2cSendBytes(const uint8_t *buf, size_t len)
{
  return i2cSendBytes(buf, len, I2C_CMD_UNKOWN);
}

bool i2cSendCmd(uint8_t addr, const uint8_t *cmd, size_t len, i2c_cmdref_t origin)
{
  if (ithoInitResult == -2)
  {
    return false;
  }

  int log_entry_idx = i2cLogger.i2cLogStart(origin);

  if (len)
  {
    esp_err_t rc = i2cMasterSendCommand(addr, cmd, len, log_entry_idx);
    if (rc)
    {
      // D_LOG("Master send: %d", rc);
      i2cLogger.i2cLogFinal(log_entry_idx, I2CLogger::I2C_NOK);
      return false;
    }
    i2cLogger.i2cLogFinal(log_entry_idx, I2CLogger::I2C_OK);
    return true;
  }
  i2cLogger.i2cLogFinal(log_entry_idx, I2CLogger::I2C_ERROR_LEN);
  return false;
}

bool i2cSendCmd(uint8_t addr, const uint8_t *cmd, size_t len)
{
  return i2cSendCmd(addr, cmd, len, I2C_CMD_UNKOWN);
}

size_t i2cSlaveReceive(uint8_t *i2c_receive_buf)
{
  i2c_config_t conf_slave = {
      .mode = I2C_MODE_SLAVE,
      .sda_io_num = i2cManager.slave_sda_pin,
      .scl_io_num = i2cManager.slave_scl_pin,
      .sda_pullup_en = I2C_SLAVE_SDA_PULLUP,
      .scl_pullup_en = I2C_SLAVE_SCL_PULLUP,
      .slave = {
          .addr_10bit_en = 0,
          .slave_addr = I2C_SLAVE_ADDRESS,
          .maximum_speed = 400000UL,
      },
      .clk_flags = I2C_SCLK_SRC_FLAG_FOR_NOMAL, // optional
  };

  i2c_param_config(I2C_SLAVE_NUM, &conf_slave);
  i2c_driver_install(I2C_SLAVE_NUM, conf_slave.mode, I2C_SLAVE_RX_BUF_LEN, I2C_SLAVE_TX_BUF_LEN, 0);
  i2c_set_timeout(I2C_SLAVE_NUM, 0xFFFFF);

  i2c_receive_buf[0] = I2C_SLAVE_ADDRESS << 1;

  // Read I2C response in a loop to handle devices that send data in multiple chunks
  // (e.g., HRU 300). Some devices may send responses across multiple I2C transactions
  // or with delays between bytes.
  int total_len = 0;
  while (total_len < I2C_SLAVE_RX_BUF_LEN - 1)
  {
    int len = i2c_slave_read_buffer(I2C_SLAVE_NUM, &i2c_receive_buf[1 + total_len],
                                     I2C_SLAVE_RX_BUF_LEN - 1 - total_len,
                                     50 / portTICK_PERIOD_MS);
    if (len <= 0)
      break;
    total_len += len;
  }

  i2cSlaveDeinit();

  if (total_len <= 0)
    return 0;

  return total_len + 1; // add 1 because i2c_receive_buf[0] = I2C_SLAVE_ADDRESS
}

void i2cSlaveDeinit()
{
  i2c_driver_delete(I2C_SLAVE_NUM);
}

bool checkI2Cbus(int log_entry_idx)
{

  if (currentIthoDeviceGroup() == 0x00 && currentIthoDeviceID() == 0x0D) // skip i2c check for WPU devices
    return true;

  bool scl_high = digitalRead(i2cManager.master_scl_pin) == HIGH;
  bool sda_high = digitalRead(i2cManager.master_sda_pin) == HIGH;

  if (scl_high && sda_high)
  {
    return true;
  }
  // else
  // {
  //   if (!scl_high && !sda_high)
  //   {
  //     i2cLogger.i2cLogErrState(log_entry_idx, I2CLogger::I2C_ERROR_SCL_SDA_LOW);
  //   }
  //   else if (!scl_high)
  //   {
  //     i2cLogger.i2cLogErrState(log_entry_idx, I2CLogger::I2C_ERROR_SCL_LOW);
  //   }
  //   else
  //   {
  //     i2cLogger.i2cLogErrState(log_entry_idx, I2CLogger::I2C_ERROR_SDA_LOW);
  //   }

  // }

  // read I2C pin state as fast as possible and return if both are HIGH for 100uS (about 10 I2C clock periods at 100kHz)
  // Using GPIO.in directly seems to be 3x faster compared to DigitalRead
  // source: https://www.reddit.com/r/esp32/comments/f529hf/results_comparing_the_speeds_of_different_gpio/
  // As master_scl_pin and master_sda_pin are within the range GPIO0~31, GPIO.in can be used
  unsigned long pin_check_timout = micros();
  while (digitalRead(i2cManager.master_scl_pin) == HIGH && digitalRead(i2cManager.master_sda_pin) == HIGH)
  {
    if (micros() - pin_check_timout < 100)
      return true;
  }

  unsigned int timeout = 200;
  unsigned int startread = millis();
  // unsigned int startbusy = millis();

  unsigned int cntr = 0;
  // bool log_i2cbus_busy = false;

  while ((digitalRead(i2cManager.master_scl_pin) == LOW || digitalRead(i2cManager.master_sda_pin) == LOW) && cntr < 10)
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
    W_LOG("I2C: warning - timeout, trying I2C bus reset...");
    int result = i2cClearBus();
    if (result != 0)
    {
      if (result == 1)
      {
        i2cLogger.i2cLogErrState(log_entry_idx, I2CLogger::I2C_ERROR_SCL_LOW);
      }
      else if (result == 2)
      {
        i2cLogger.i2cLogErrState(log_entry_idx, I2CLogger::I2C_ERROR_SLAVE_CLOCKSTRETCH);
      }
      else if (result == 3)
      {
        i2cLogger.i2cLogErrState(log_entry_idx, I2CLogger::I2C_ERROR_SDA_LOW);
      }
      E_LOG("I2C: error - I2C bus could not be cleared!");
      if (systemConfig.i2cmenu == 1)
      {
        ithoInitResult = -2; // stop I2C bus activity to be able to save logging
      }
    }
    else
    {
      i2cLogger.i2cLogErrState(log_entry_idx, I2CLogger::I2C_ERROR_CLEARED_BUS);
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
    //   N_LOG(buf);
    // }
    i2cLogger.i2cLogErrState(log_entry_idx, I2CLogger::I2C_ERROR_CLEARED_OK);
    return true;
  }
}

/**
 *
 * Credits for this function: http://www.forward.com.au/pfod/ArduinoProgramming/i2cClearBus/index.html
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
int i2cClearBus()
{
  // Remove any existing I2C drivers
  i2cMasterDeinit();
  i2cSlaveDeinit();

  pinMode(i2cManager.master_sda_pin, INPUT_PULLUP); // Make SDA (data) and SCL (clock) pins Inputs with pullup.
  pinMode(i2cManager.master_scl_pin, INPUT_PULLUP);

  delay(2500); // Wait 2.5 secs. This is strictly only necessary on the first power
  // up of the DS3231 module to allow it to initialize properly,
  // but is also assists in reliable programming of FioV3 boards as it gives the
  // IDE a chance to start uploaded the program
  // before existing sketch confuses the IDE by sending Serial data.

  // i2c_master_clear_bus(I2C_MASTER_NUM);

  bool SCL_LOW = (digitalRead(i2cManager.master_scl_pin) == LOW); // Check is SCL is Low.
  if (SCL_LOW)
  {           // If it is held low Arduno cannot become the I2C master.
    return 1; // I2C bus error. Could not clear SCL clock line held low
  }

  bool SDA_LOW = (digitalRead(i2cManager.master_sda_pin) == LOW); // vi. Check SDA input.
  int clockCount = 20;                                 // > 2x9 clock

  while (SDA_LOW && (clockCount > 0))
  { //  vii. If SDA is Low,
    clockCount--;
    // Note: I2C bus is open collector so do NOT drive SCL or SDA high.
    pinMode(i2cManager.master_scl_pin, INPUT);        // release SCL pullup so that when made output it will be LOW
    pinMode(i2cManager.master_scl_pin, OUTPUT);       // then clock SCL Low
    delayMicroseconds(10);                 //  for >5us
    pinMode(i2cManager.master_scl_pin, INPUT);        // release SCL LOW
    pinMode(i2cManager.master_scl_pin, INPUT_PULLUP); // turn on pullup resistors again
    // do not force high as slave may be holding it low for clock stretching.
    delayMicroseconds(10); //  for >5us
    // The >5us is so that even the slowest I2C devices are handled.
    SCL_LOW = (digitalRead(i2cManager.master_scl_pin) == LOW); // Check if SCL is Low.
    int counter = 20;
    while (SCL_LOW && (counter > 0))
    { //  loop waiting for SCL to become High only wait 2sec.
      counter--;
      delay(100);
      SCL_LOW = (digitalRead(i2cManager.master_scl_pin) == LOW);
    }
    if (SCL_LOW)
    {           // still low after 2 sec error
      return 2; // I2C bus error. Could not clear. SCL clock line held low by slave clock stretch for >2sec
    }
    SDA_LOW = (digitalRead(i2cManager.master_sda_pin) == LOW); //   and check SDA input again and loop
  }
  if (SDA_LOW)
  {           // still low
    return 3; // I2C bus error. Could not clear. SDA data line held low
  }

  // else pull SDA line low for Start or Repeated Start
  pinMode(i2cManager.master_sda_pin, INPUT);  // remove pullup.
  pinMode(i2cManager.master_sda_pin, OUTPUT); // and then make it LOW i.e. send an I2C Start or Repeated start control.
  // When there is only one I2C master a Start or Repeat Start has the same function as a Stop and clears the bus.
  /// A Repeat Start is a Start occurring after a Start with no intervening Stop.
  delayMicroseconds(10);                 // wait >5us
  pinMode(i2cManager.master_sda_pin, INPUT);        // remove output low
  pinMode(i2cManager.master_sda_pin, INPUT_PULLUP); // and make SDA high i.e. send I2C STOP control.
  delayMicroseconds(10);                 // x. wait >5us
  pinMode(i2cManager.master_sda_pin, INPUT);        // and reset pins as tri-state inputs which is the default state on reset
  pinMode(i2cManager.master_scl_pin, INPUT);
  return 0; // all ok
}

void triggerShtSensorReset()
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
  i2cMasterDeinit();
  i2cSlaveDeinit();

  pinMode(i2cManager.master_sda_pin, OUTPUT); // Make SDA (data) and SCL (clock) pins outputs
  pinMode(i2cManager.master_scl_pin, OUTPUT);

  digitalWrite(i2cManager.master_sda_pin, HIGH);
  digitalWrite(i2cManager.master_scl_pin, HIGH);

  for (uint i = 0; i < 10; i++)
  {
    digitalWrite(i2cManager.master_scl_pin, LOW);
    delayMicroseconds(10);
    digitalWrite(i2cManager.master_scl_pin, HIGH);
    delayMicroseconds(10);
  }

  pinMode(i2cManager.master_sda_pin, INPUT); // and reset pins as tri-state inputs which is the default state on reset
  pinMode(i2cManager.master_scl_pin, INPUT);

  delayMicroseconds(20);
}