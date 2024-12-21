#include "i2c_esp32.h"

std::deque<std::function<void()>> i2c_cmd_queue;

void i2c_queue_add_cmd(const std::function<void()> func)
{
  if (i2c_cmd_queue.size() >= I2C_CMD_QUEUE_MAX_SIZE)
  {
    log_msg input;
    input.code = SYSLOG_WARNING;
    input.msg = "Warning: i2c_cmd_queue overflow";
    syslog_queue.push_back(input);
    return;
  }
  i2c_cmd_queue.push_back(func);
}

gpio_num_t master_sda_pin = GPIO_NUM_0;
gpio_num_t master_scl_pin = GPIO_NUM_0;

gpio_num_t slave_sda_pin = GPIO_NUM_0;
gpio_num_t slave_scl_pin = GPIO_NUM_0;

void i2c_master_setpins(gpio_num_t sda, gpio_num_t scl)
{
  master_sda_pin = sda;
  master_scl_pin = scl;
}

void i2c_slave_setpins(gpio_num_t sda, gpio_num_t scl)
{
  slave_sda_pin = sda;
  slave_scl_pin = scl;
}

esp_err_t i2c_master_init(int log_entry_idx)
{

  esp_err_t result;

  if (!checkI2Cbus(log_entry_idx))
    result = ESP_FAIL;

  i2c_config_t conf = {
      .mode = I2C_MODE_MASTER,
      .sda_io_num = master_sda_pin,
      .scl_io_num = master_scl_pin,
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
    E_LOG("i2c_master_init error: %s", esp_err_to_name(result));
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
  rc = i2c_master_cmd_begin(I2C_MASTER_NUM, link, 200 / portTICK_PERIOD_MS);
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
  rc = i2c_master_cmd_begin(I2C_MASTER_NUM, link, 200 / portTICK_PERIOD_MS);

  i2c_cmd_link_delete(link);

  i2c_master_deinit();

  return rc;
}

esp_err_t i2c_master_read_slave(uint8_t addr, uint8_t *data_rd, size_t size, i2c_cmdref_t origin)
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
  return i2c_master_read_slave(addr, data_rd, size, I2C_CMD_UNKOWN);
}

bool i2c_sendBytes(const uint8_t *buf, size_t len, i2c_cmdref_t origin)
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
      // D_LOG("Master send: %d", rc);
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
  return i2c_sendBytes(buf, len, I2C_CMD_UNKOWN);
}

bool i2c_sendCmd(uint8_t addr, const uint8_t *cmd, size_t len, i2c_cmdref_t origin)
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
      // D_LOG("Master send: %d", rc);
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
  return i2c_sendCmd(addr, cmd, len, I2C_CMD_UNKOWN);
}

size_t i2c_slave_receive(uint8_t *i2c_receive_buf)
{
  i2c_config_t conf_slave = {
      .mode = I2C_MODE_SLAVE,
      .sda_io_num = slave_sda_pin,
      .scl_io_num = slave_scl_pin,
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

  // uint8_t byte = 0;
  // int len_rec = 0;
  // attempt to read in bytes one by one
  // while (true)
  // {
  //   int len = i2c_slave_read_buffer(I2C_SLAVE_NUM, &byte, 1, 200 / portTICK_PERIOD_MS);
  //   if (len <= 0)
  //     break;
  //   i2c_receive_buf[len_rec++] = byte;
  // }

  int len_rec = i2c_slave_read_buffer(I2C_SLAVE_NUM, &i2c_receive_buf[1], I2C_SLAVE_RX_BUF_LEN - 1, 200 / portTICK_PERIOD_MS);
  if (len_rec <= 0)
    return 0;

  i2c_slave_deinit();
  return len_rec + 1; // add 1 because i2c_receive_buf[0] = I2C_SLAVE_ADDRESS
}

void i2c_slave_deinit()
{
  i2c_driver_delete(I2C_SLAVE_NUM);
}

bool checkI2Cbus(int log_entry_idx)
{

  bool scl_high = digitalRead(master_scl_pin) == HIGH;
  bool sda_high = digitalRead(master_sda_pin) == HIGH;

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
  // As master_scl_pin and master_sda_pin are within the range GPIO0~31, GPIO.in can be used
  unsigned long pin_check_timout = micros();
  while (digitalRead(master_scl_pin) == HIGH && digitalRead(master_sda_pin) == HIGH)
  {
    if (micros() - pin_check_timout < 100)
      return true;
  }

  unsigned int timeout = 200;
  unsigned int startread = millis();
  // unsigned int startbusy = millis();

  unsigned int cntr = 0;
  // bool log_i2cbus_busy = false;

  while ((digitalRead(master_scl_pin) == LOW || digitalRead(master_sda_pin) == LOW) && cntr < 10)
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
    W_LOG("Warning: I2C timeout, trying I2C bus reset...");
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
      E_LOG("Error: I2C bus could not be cleared!");
      if (systemConfig.i2cmenu == 1)
      {
        ithoInitResult = -2; // stop I2C bus activity to be able to save logging
      }
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
    //   N_LOG(buf);
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

  pinMode(master_sda_pin, INPUT_PULLUP); // Make SDA (data) and SCL (clock) pins Inputs with pullup.
  pinMode(master_scl_pin, INPUT_PULLUP);

  delay(2500); // Wait 2.5 secs. This is strictly only necessary on the first power
  // up of the DS3231 module to allow it to initialize properly,
  // but is also assists in reliable programming of FioV3 boards as it gives the
  // IDE a chance to start uploaded the program
  // before existing sketch confuses the IDE by sending Serial data.

  // i2c_master_clear_bus(I2C_MASTER_NUM);

  bool SCL_LOW = (digitalRead(master_scl_pin) == LOW); // Check is SCL is Low.
  if (SCL_LOW)
  {           // If it is held low Arduno cannot become the I2C master.
    return 1; // I2C bus error. Could not clear SCL clock line held low
  }

  bool SDA_LOW = (digitalRead(master_sda_pin) == LOW); // vi. Check SDA input.
  int clockCount = 20;                                 // > 2x9 clock

  while (SDA_LOW && (clockCount > 0))
  { //  vii. If SDA is Low,
    clockCount--;
    // Note: I2C bus is open collector so do NOT drive SCL or SDA high.
    pinMode(master_scl_pin, INPUT);        // release SCL pullup so that when made output it will be LOW
    pinMode(master_scl_pin, OUTPUT);       // then clock SCL Low
    delayMicroseconds(10);                 //  for >5us
    pinMode(master_scl_pin, INPUT);        // release SCL LOW
    pinMode(master_scl_pin, INPUT_PULLUP); // turn on pullup resistors again
    // do not force high as slave may be holding it low for clock stretching.
    delayMicroseconds(10); //  for >5us
    // The >5us is so that even the slowest I2C devices are handled.
    SCL_LOW = (digitalRead(master_scl_pin) == LOW); // Check if SCL is Low.
    int counter = 20;
    while (SCL_LOW && (counter > 0))
    { //  loop waiting for SCL to become High only wait 2sec.
      counter--;
      delay(100);
      SCL_LOW = (digitalRead(master_scl_pin) == LOW);
    }
    if (SCL_LOW)
    {           // still low after 2 sec error
      return 2; // I2C bus error. Could not clear. SCL clock line held low by slave clock stretch for >2sec
    }
    SDA_LOW = (digitalRead(master_sda_pin) == LOW); //   and check SDA input again and loop
  }
  if (SDA_LOW)
  {           // still low
    return 3; // I2C bus error. Could not clear. SDA data line held low
  }

  // else pull SDA line low for Start or Repeated Start
  pinMode(master_sda_pin, INPUT);  // remove pullup.
  pinMode(master_sda_pin, OUTPUT); // and then make it LOW i.e. send an I2C Start or Repeated start control.
  // When there is only one I2C master a Start or Repeat Start has the same function as a Stop and clears the bus.
  /// A Repeat Start is a Start occurring after a Start with no intervening Stop.
  delayMicroseconds(10);                 // wait >5us
  pinMode(master_sda_pin, INPUT);        // remove output low
  pinMode(master_sda_pin, INPUT_PULLUP); // and make SDA high i.e. send I2C STOP control.
  delayMicroseconds(10);                 // x. wait >5us
  pinMode(master_sda_pin, INPUT);        // and reset pins as tri-state inputs which is the default state on reset
  pinMode(master_scl_pin, INPUT);
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

  pinMode(master_sda_pin, OUTPUT); // Make SDA (data) and SCL (clock) pins outputs
  pinMode(master_scl_pin, OUTPUT);

  digitalWrite(master_sda_pin, HIGH);
  digitalWrite(master_scl_pin, HIGH);

  for (uint i = 0; i < 10; i++)
  {
    digitalWrite(master_scl_pin, LOW);
    delayMicroseconds(10);
    digitalWrite(master_scl_pin, HIGH);
    delayMicroseconds(10);
  }

  pinMode(master_sda_pin, INPUT); // and reset pins as tri-state inputs which is the default state on reset
  pinMode(master_scl_pin, INPUT);

  delayMicroseconds(20);
}