#include "globals.h"


bool i2c_sniffer_capable = false;
uint8_t hardware_rev_det = 0;

gpio_num_t wifi_led_pin = GPIO_NUM_17;
gpio_num_t status_pin = GPIO_NUM_16;
gpio_num_t itho_irq_pin = GPIO_NUM_4;
gpio_num_t boot_state_pin = GPIO_NUM_0;
gpio_num_t fail_save_pin = GPIO_NUM_0;
// gpio_num_t itho_status_pin = GPIO_NUM_0;

const char *cve2 = "2";
const char *non_cve1 = "NON-CVE 1";
const char *hw_revision = nullptr;

WiFiClient client;

IthoCC1101 rf;
IthoPacket packet;
