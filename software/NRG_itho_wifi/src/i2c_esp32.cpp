#include "i2c_esp32.h"

char toHex(uint8_t c)
{
  return c < 10 ? c + '0' : c + 'A' - 10;
}

uint8_t i2cbuf[I2C_SLAVE_RX_BUF_LEN];
static size_t buflen;

void i2c_master_init()
{

  int i2c_master_port = I2C_MASTER_NUM;

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

  i2c_param_config(i2c_master_port, &conf);

  esp_err_t result = i2c_driver_install(i2c_master_port, conf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
  if (result != ESP_OK)
  {
    D_LOG("i2c_master_init error: %s\n", esp_err_to_name(result));
  }
}

void i2c_master_deinit()
{
  i2c_driver_delete(I2C_MASTER_NUM);
}

esp_err_t i2c_master_send(const char *buf, uint32_t len)
{
  i2c_master_init();

  esp_err_t rc;
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

esp_err_t i2c_master_send_command(uint8_t addr, const uint8_t *cmd, uint32_t len)
{

    i2c_master_init();

    esp_err_t rc;
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

esp_err_t i2c_master_read_slave(uint8_t addr, uint8_t *data_rd, size_t size)
{

  i2c_master_init();

  esp_err_t rc;
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

  return rc;
}

bool i2c_sendBytes(const uint8_t *buf, size_t len)
{

  if (len)
  {
    esp_err_t rc = i2c_master_send((const char *)buf, len);
    if (rc)
    {
      D_LOG("Master send: %d\n", rc);
      return false;
    }
    return true;
  }
  return false;
}

bool i2c_sendCmd(uint8_t addr, const uint8_t *cmd, size_t len)
{
  if (len)
  {
    esp_err_t rc = i2c_master_send_command(addr, cmd, len);
    if (rc)
    {
      D_LOG("Master send: %d\n", rc);
      return false;
    }
    return true;
  }
  return false;
}

size_t i2c_slave_receive(uint8_t i2c_receive_buf[])
{

  int i2c_slave_port = I2C_SLAVE_NUM;
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

  i2c_param_config(i2c_slave_port, &conf_slave);

  i2c_driver_install(i2c_slave_port, conf_slave.mode, I2C_SLAVE_RX_BUF_LEN, I2C_SLAVE_TX_BUF_LEN, 0);

  i2c_set_timeout(i2c_slave_port, 0xFFFFF);

  i2cbuf[0] = I2C_SLAVE_ADDRESS << 1;
  buflen = 1;

  while (1)
  {
    int len1 = i2c_slave_read_buffer(i2c_slave_port, i2cbuf + buflen, sizeof(i2cbuf) - buflen, 50 / portTICK_RATE_MS);
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
