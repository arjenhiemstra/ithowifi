
#include "System.h"

System sys;

const char* System::uptime()
{
  long secs = millis() / 1000;
  long mins = secs / 60;
  long hours = mins / 60;
  long days = hours / 24;
  secs %= 60;
  mins %= 60;
  hours %= 24;

  snprintf(retval, sizeof(retval), "%02ld:%02ld:%02ld:%02ld", days, hours, mins, secs);

  return retval;
}

uint8_t System::getMac(uint8_t i) {
  uint8_t mac[6];

  esp_read_mac(mac, ESP_MAC_WIFI_STA);

  return mac[i];
}

int System::ramFree() {
  int a = ESP.getFreeHeap();
  return a;
}

#if defined (ESP8266)
int System::ramSize() {
  return 0;
}
#elif defined (ESP32)
int System::ramSize() {
  multi_heap_info_t info;
  heap_caps_get_info(&info, MALLOC_CAP_INTERNAL);
  return info.total_free_bytes;
}
#endif

bool System::updateFreeMem() {
  bool result = false;
#if defined (ESP8266)
  int newMem = ramFree();
#elif defined (ESP32)
  int newMem = ramSize();
#endif
  if (newMem != memHigh) {
    memHigh = newMem;
    result = true;
  }
#if defined (ESP32)
  newMem = heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL);
#endif  
  if (newMem < memLow) {
    memLow = newMem;
    result = true;
  }
  return result;
}

int System::getMemHigh() {
  return memHigh;
}

int System::getMemLow() {
  return memLow;
}

uint32_t System::getMaxFreeBlockSize() {

#if defined (ESP8266)
  memMaxBlock = ESP.getMaxFreeBlockSize();
#elif defined (ESP32)
  memMaxBlock = ESP.getMaxAllocHeap();
#endif  
  return memMaxBlock;
}
