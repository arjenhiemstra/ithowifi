
#include "SystemConfig.h"
#include "IthoSystem.h"
#include "hardware.h"
#include "i2c_esp32.h"
#include "notifyClients.h"
#include "IthoSystemConsts.h"

uint8_t ithoDeviceID = 0;
uint8_t itho_fwversion = 0;
volatile uint16_t ithoCurrentVal = 0;
uint8_t id0 = 0;
uint8_t id1 = 0;
uint8_t id2 = 0;
struct ihtoDeviceType* ithoDeviceptr = getDevicePtr(ithoDeviceID);
int ithoSettingsLength = getSettingsLength(ithoDeviceID, itho_fwversion);
std::vector<ithoDeviceStatus> ithoStatus;
std::vector<ithoDeviceMeasurements> ithoMeasurements;
std::vector<ithoDeviceMeasurements> ithoInternalMeasurements;
struct lastCommand lastCmd;

const std::map<cmdOrigin, const char*> cmdOriginMap = {
  {cmdOrigin::HTMLAPI, "HTML API"},
  {cmdOrigin::MQTTAPI, "MQTT API"},
  {cmdOrigin::REMOTE, "remote"},
  {cmdOrigin::WEB, "web interface"},
  {cmdOrigin::UNKNOWN, "unknown"}
};

bool sendI2CButton = false;
bool sendI2CTimer = false;
bool sendI2CJoin = false;
bool sendI2CLeave = false;
bool sendI2CDevicetype = false;
bool sendI2CStatusFormat = false;
bool sendI2CStatus = false;
bool send31DA = false;
bool send31D9 = false;
bool get2410 = false;
bool set2410 = false;
bool buttonResult = false;
uint8_t buttonValue = 0;
uint8_t timerValue = 0;
uint8_t index2410 = 0;
int32_t value2410 = 0;
int32_t * resultPtr2410 = nullptr;
bool i2c_result_updateweb = false;

bool itho_internal_hum_temp = false;
float ithoHum = 0;
float ithoTemp = 0;

Ticker getSettingsHack;
SemaphoreHandle_t mutexI2Ctask;

struct ihtoDeviceType {
  uint8_t ID;
  const char *name;
  const uint8_t **settingsMapping;
  uint8_t versionsLen;
  const __FlashStringHelper **settingsDescriptions;
  const uint8_t **statusLabelMapping;
  const __FlashStringHelper **settingsStatusLabels;

};


struct ihtoDeviceType ithoDevices[] {
  { 0x01, "Air curtain",        nullptr, 0, nullptr, nullptr, nullptr },
  { 0x03, "HRU ECO-fan",        ithoHRUecoFanSettingsMap, sizeof(ithoHRUecoFanSettingsMap) / sizeof(ithoHRUecoFanSettingsMap[0]), ihtoHRUecoFanSettingsDescriptions, ithoHRUecoFanStatusMap, ihtoHRUecoFanStatusLabels },
  { 0x08, "LoadBoiler",         nullptr, 0, nullptr, nullptr, nullptr },
  { 0x0A, "GGBB",               nullptr, 0, nullptr, nullptr, nullptr },
  { 0x0B, "DemandFlow",         ithoDemandFlowSettingsMap, sizeof(ithoDemandFlowSettingsMap) / sizeof(ithoDemandFlowSettingsMap[0]), ihtoDemandFlowSettingsDescriptions, ithoDemandFlowStatusMap, ihtoDemandFlowStatusLabels  },
  { 0x0C, "CO2 relay",          nullptr, 0, nullptr, nullptr, nullptr },
  { 0x0D, "Heatpump",           nullptr, 0, nullptr, nullptr, nullptr },
  { 0x0E, "OLB Single",         nullptr, 0, nullptr, nullptr, nullptr },
  { 0x0F, "AutoTemp",           nullptr, 0, nullptr, nullptr, nullptr },
  { 0x10, "OLB Double",         nullptr, 0, nullptr, nullptr, nullptr },
  { 0x11, "RF+",                nullptr, 0, nullptr, nullptr, nullptr },
  { 0x14, "CVE",                itho14SettingsMap, sizeof(itho14SettingsMap) / sizeof(itho14SettingsMap[0]), ihtoCVESettingsDescriptions, itho14StatusMap, ihtoCVEStatusLabels  },
  { 0x15, "Extended",           nullptr, 0, nullptr, nullptr, nullptr },
  { 0x16, "Extended Plus",      nullptr, 0, nullptr, nullptr, nullptr },
  { 0x1A, "AreaFlow",           nullptr, 0, nullptr, nullptr, nullptr },
  { 0x1B, "CVE-Silent",         itho1BSettingsMap, sizeof(itho1BSettingsMap) / sizeof(itho1BSettingsMap[0]), ihtoCVESettingsDescriptions, itho1BStatusMap, ihtoCVEStatusLabels  },
  { 0x1C, "CVE-SilentExt",      nullptr, 0, nullptr, nullptr, nullptr },
  { 0x1D, "CVE-SilentExtPlus",  nullptr, 0, nullptr, nullptr, nullptr },
  { 0x20, "RF_CO2",             nullptr, 0, nullptr, nullptr, nullptr },
  { 0x2B, "HRU 350",            ithoHRU350SettingsMap, sizeof(ithoHRU350SettingsMap) / sizeof(ithoHRU350SettingsMap[0]), ihtoHRU350SettingsDescriptions, ithoHRU350StatusMap, ihtoHRU350StatusLabels  }
};



char* getIthoType(const uint8_t deviceID) {
  static char ithoDeviceType[32] = "Unkown device type";

  struct ihtoDeviceType* ithoDevicesptr = ithoDevices;
  struct ihtoDeviceType* ithoDevicesendPtr = ithoDevices + sizeof(ithoDevices) / sizeof(ithoDevices[0]);
  while ( ithoDevicesptr < ithoDevicesendPtr ) {
    if (ithoDevicesptr->ID == deviceID) {
      strcpy(ithoDeviceType, ithoDevicesptr->name);
    }
    ithoDevicesptr++;
  }
  return ithoDeviceType;
}

int getSettingsLength(const uint8_t deviceID, const uint8_t version) {

  struct ihtoDeviceType* ithoDevicesptr = ithoDevices;
  struct ihtoDeviceType* ithoDevicesendPtr = ithoDevices + sizeof(ithoDevices) / sizeof(ithoDevices[0]);
  while ( ithoDevicesptr < ithoDevicesendPtr ) {
    if (ithoDevicesptr->ID == deviceID) {
      if (ithoDevicesptr->settingsMapping == nullptr) {
        return -2; //Settings not available for this device
      }
      if (version > ithoDevicesptr->versionsLen) {
        return -3; //Settings not available for this version
      }
      if (*(ithoDevicesptr->settingsMapping + version) == nullptr) {
        return -3; //Settings not available for this version
      }

      for (int i = 0; i < 255; i++) {
        if ((int) * (*(ithoDevicesptr->settingsMapping + version) + i) == 255) {
          //end of array
          return i;
        }
      }
    }
    ithoDevicesptr++;
  }
  return -1;
}

void getSetting(const uint8_t i, const bool updateState, const bool updateweb, const bool loop) {
  getSetting(i, updateState, updateweb, loop, ithoDeviceptr, ithoDeviceID, itho_fwversion);
}

void getSetting(const uint8_t i, const bool updateState, const bool updateweb, const bool loop, const struct ihtoDeviceType* settingsPtr, const uint8_t deviceID, const uint8_t version) {

  int settingsLen = getSettingsLength(deviceID, version);
  if (settingsLen < 0) {
    logMessagejson(F("Settings not available for this device or its firmware version"), WEBINTERFACE);
    return;
  }

  char fDescBuf[128];
  strcpy_P( fDescBuf, (PGM_P)(settingsPtr->settingsDescriptions[(int) * (*(settingsPtr->settingsMapping + version) + i)]) );

  StaticJsonDocument<512> doc;
  JsonObject root = doc.to<JsonObject>();


  root["Index"] = i;
  root["Description"] = fDescBuf;

  if (!updateState && !updateweb) {
    root["update"] = false;
    root["loop"] = loop;
    root["Current"] = nullptr;
    root["Minimum"] = nullptr;
    root["Maximum"] = nullptr;
    logMessagejson(root, ITHOSETTINGS);
  }
  else if (updateState && !updateweb) {
    index2410 = i;
    i2c_result_updateweb = false;
    resultPtr2410 = nullptr;
    get2410 = true;

    auto timeoutmillis = millis() + 1000;
    while (resultPtr2410 == nullptr && millis() < timeoutmillis) {
      //wait for result
    }
    root["update"] = true;
    root["loop"] = loop;
    if (resultPtr2410 != nullptr) {
      root["Current"] = *(resultPtr2410 + 0);
      root["Minimum"] = *(resultPtr2410 + 1);
      root["Maximum"] = *(resultPtr2410 + 2);
    }
    else {
      root["Current"] = nullptr;
      root["Minimum"] = nullptr;
      root["Maximum"] = nullptr;
    }
    logMessagejson(root, ITHOSETTINGS);
  }
  else {
    index2410 = i;
    i2c_result_updateweb = updateweb;
    resultPtr2410 = nullptr;
    get2410 = true;
  }

}

void getSatusLabel(const uint8_t i, const struct ihtoDeviceType* statusPtr, const uint8_t version, char* fStringBuf) {

  if (statusPtr == nullptr) {
    strcpy(fStringBuf, "nullptr error");
    return;
  }
  strcpy_P(fStringBuf, (PGM_P)(statusPtr->settingsStatusLabels[(int) * (*(statusPtr->statusLabelMapping + version) + i)]) );

}

void updateSetting(const uint8_t i, const int32_t value, bool webupdate) {
#if defined (HW_VERSION_TWO)
  if (xSemaphoreTake(mutexI2Ctask, (TickType_t) 500 / portTICK_PERIOD_MS) == pdTRUE) {
#endif
    i2c_result_updateweb = webupdate;
    index2410 = i;
    value2410 = value;
    set2410 = true;
  }
}

struct ihtoDeviceType* getDevicePtr(uint8_t deviceID) {

  struct ihtoDeviceType* ithoDevicesptr = ithoDevices;
  struct ihtoDeviceType* ithoDevicesendPtr = ithoDevices + sizeof(ithoDevices) / sizeof(ithoDevices[0]);
  while ( ithoDevicesptr < ithoDevicesendPtr ) {
    if (ithoDevicesptr->ID == deviceID) {
      return ithoDevicesptr;
    }
    ithoDevicesptr++;
  }
  return nullptr;
}



uint8_t checksum(const uint8_t* buf, size_t buflen) {
  uint8_t sum = 0;
  while (buflen--) {
    sum += *buf++;
  }
  return -sum;
}


void sendI2CPWMinit() {

  uint8_t command[] = { 0x82, 0xEF, 0xC0, 0x00, 0x01, 0x06, 0x00, 0x00, 0x09, 0x00, 0x09, 0x00, 0xB6 };

  command[sizeof(command) - 1] = checksum(command, sizeof(command) - 1);

  while (digitalRead(SCLPIN) == LOW) {
    yield();
    delay(1);
  }

#if defined (HW_VERSION_TWO)

  i2c_sendBytes(command, sizeof(command));

#endif

}

uint8_t cmdCounter = 0;

void sendButton(uint8_t number, bool & updateweb) {

  uint8_t command[] = { 0x82, 0x60, 0xC1, 0x01, 0x01, 0x11, 0x00, 0x00, 0x00, 0x00, 0x16, 0xFF, 0xFF, 0xFF, 0xFF, 0x22, 0xF1, 0x03, 0x00, 0x01, 0x04, 0x00, 0x00, 0xFF };

  command[11] = id0;
  command[12] = id1;
  command[13] = id2;

  command[14] = cmdCounter;
  cmdCounter++;

  command[19] += number;

  command[sizeof(command) - 1] = checksum(command, sizeof(command) - 1);

  while (digitalRead(SCLPIN) == LOW ) {
    yield();
    delay(1);
  }

#if defined (HW_VERSION_ONE)
  Wire.beginTransmission(byte(0x41));
  for (uint8_t i = 1; i < sizeof(command); i++) {
    Wire.write(command[i]);
  }
  Wire.endTransmission(true);
#elif defined (HW_VERSION_TWO)
  i2c_sendBytes(command, sizeof(command));
#endif
  buttonResult = true;

}

void sendTimer(uint8_t timer, bool & updateweb) {

  uint8_t command[] = { 0x82, 0x60, 0xC1, 0x01, 0x01, 0x11, 0x00, 0x00, 0x00, 0x00, 0x16, 0xFF, 0xFF, 0xFF, 0xFF, 0x22, 0xF3, 0x03, 0x00, 0x00, 0xFF, 0x00, 0x00, 0xFF };

  command[11] = id0;
  command[12] = id1;
  command[13] = id2;

  command[14] = cmdCounter;
  cmdCounter++;

  command[20] = timer;

  command[sizeof(command) - 1] = checksum(command, sizeof(command) - 1);

  while (digitalRead(SCLPIN) == LOW ) {
    yield();
    delay(1);
  }

#if defined (HW_VERSION_ONE)
  Wire.beginTransmission(byte(0x41));
  for (uint8_t i = 1; i < sizeof(command); i++) {
    Wire.write(command[i]);
  }
  Wire.endTransmission(true);
#elif defined (HW_VERSION_TWO)
  i2c_sendBytes(command, sizeof(command));
#endif

}

void sendJoinI2C(bool & updateweb) {

  uint8_t command[] = { 0x82, 0x60, 0xC1, 0x01, 0x01, 0x1A, 0x00, 0x00, 0x00, 0x00, 0x16, 0xFF, 0xFF, 0xFF, 0xFF, 0x1F, 0xC9, 0x0C, 0x00, 0x22, 0xF1, 0xFF, 0xFF, 0xFF, 0x01, 0x10, 0xE0, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0xFF };

  command[11] = id0;
  command[12] = id1;
  command[13] = id2;

  command[14] = cmdCounter;
  cmdCounter++;

  command[21] = id0;
  command[22] = id1;
  command[23] = id2;

  command[27] = id0;
  command[28] = id1;
  command[29] = id2;

  command[sizeof(command) - 1] = checksum(command, sizeof(command) - 1);

  while (digitalRead(SCLPIN) == LOW ) {
    yield();
    delay(1);
  }

#if defined (HW_VERSION_ONE)
  Wire.beginTransmission(byte(0x41));
  for (uint8_t i = 1; i < sizeof(command); i++) {
    Wire.write(command[i]);
  }
  Wire.endTransmission(true);
#elif defined (HW_VERSION_TWO)
  i2c_sendBytes(command, sizeof(command));
#endif


}

void sendLeaveI2C(bool & updateweb) {

  uint8_t command[] = { 0x82, 0x60, 0xC1, 0x01, 0x01, 0x14, 0x00, 0x00, 0x00, 0x00, 0x16, 0xFF, 0xFF, 0xFF, 0xFF, 0x1F, 0xC9, 0x06, 0x00, 0x1F, 0xC9, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0xFF };


  command[11] = id0;
  command[12] = id1;
  command[13] = id2;

  command[14] = cmdCounter;
  cmdCounter++;

  command[21] = id0;
  command[22] = id1;
  command[23] = id2;

  command[sizeof(command) - 1] = checksum(command, sizeof(command) - 1);

  while (digitalRead(SCLPIN) == LOW ) {
    yield();
    delay(1);
  }

#if defined (HW_VERSION_ONE)
  Wire.beginTransmission(byte(0x41));
  for (uint8_t i = 1; i < sizeof(command); i++) {
    Wire.write(command[i]);
  }
  Wire.endTransmission(true);
#elif defined (HW_VERSION_TWO)
  i2c_sendBytes(command, sizeof(command));
#endif

}

void sendQueryDevicetype(bool & updateweb) {

  uint8_t command[] = { 0x82, 0x80, 0x90, 0xE0, 0x04, 0x00, 0x8A };

  while (digitalRead(SCLPIN) == LOW ) {
    yield();
    delay(1);
  }

#if defined (HW_VERSION_ONE)

#endif

#if defined (HW_VERSION_TWO)
  i2c_sendBytes(command, sizeof(command));

  uint8_t i2cbuf[512] {};

  size_t len = i2c_slave_receive(i2cbuf);

  //if (len > 2) {
  if (len > 1 && i2cbuf[len - 1] == checksum(i2cbuf, len - 1)) {
    if (updateweb) {
      updateweb = false;

      jsonSysmessage("ithotype", i2cbuf2string(i2cbuf, len).c_str());
    }

    ithoDeviceID = i2cbuf[9];
    itho_fwversion = i2cbuf[11];
    ithoDeviceptr = getDevicePtr(ithoDeviceID);
    ithoSettingsLength = getSettingsLength(ithoDeviceID, itho_fwversion);
  }
  else {
    if (updateweb) {
      updateweb = false;
      jsonSysmessage("ithotype", "failed");
    }
  }

#endif

}

void sendQueryStatusFormat(bool & updateweb) {

  uint8_t command[] = { 0x82, 0x80, 0x24, 0x00, 0x04, 0x00, 0xD6 };

  while (digitalRead(SCLPIN) == LOW ) {
    yield();
    delay(1);
  }

#if defined (HW_VERSION_ONE)
  //  Wire.beginTransmission(byte(0x41));
  //  for (uint8_t i = 1; i < sizeof(command); i++) {
  //    Wire.write(command[i]);
  //  }
  //  Wire.endTransmission(true);
  //
  //  unsigned long timeoutmillis = millis() + 1000;
  //
  //  while (!callback_called && millis() < timeoutmillis) {
  //    if (callback_called) {
  //      callback_called = false;
  //      jsonSysmessage("itho2400", i2c_slave_buf);
  //
  //    }
  //    else {
  //      jsonSysmessage("itho2400", "failed");
  //    }
  //  }

#endif
#if defined (HW_VERSION_TWO)
  i2c_sendBytes(command, sizeof(command));

  uint8_t i2cbuf[512] {};
  size_t len = i2c_slave_receive(i2cbuf);
  if (len > 1 && i2cbuf[len - 1] == checksum(i2cbuf, len - 1)) {
    if (updateweb) {
      updateweb = false;
      jsonSysmessage("ithostatusformat", i2cbuf2string(i2cbuf, len).c_str());
    }

    if (!ithoStatus.empty()) {
      ithoStatus.clear();
    }
    if (!(itho_fwversion > 0)) return;

    for (uint8_t i = 0; i < i2cbuf[5]; i++) {
      ithoStatus.push_back(ithoDeviceStatus());

      char fStringBuf[32];
      getSatusLabel(i, ithoDeviceptr, itho_fwversion, fStringBuf);

      ithoStatus.back().name.assign(fStringBuf);

      ithoStatus.back().divider = 0;
      if ((i2cbuf[6 + i] & 0x07) == 0) { //integer value
        if ((i2cbuf[6 + i] & 0x80) == 1) { //signed value
          ithoStatus.back().type = ithoDeviceStatus::is_int;
        }
        else {
          ithoStatus.back().type = ithoDeviceStatus::is_uint;
        }
      }
      else {
        ithoStatus.back().type = ithoDeviceStatus::is_float;
        ithoStatus.back().divider = quick_pow10((i2cbuf[6 + i] & 0x07));
      }
      if (((i2cbuf[6 + i] >> 3) & 0x07) == 0) {
        ithoStatus.back().length = 1;
      }
      else {
        ithoStatus.back().length = (i2cbuf[6 + i] >> 3) & 0x07;
      }

      //special cases
      if (i2cbuf[6 + i] == 0x0C || i2cbuf[6 + i] == 0x6C) {
        ithoStatus.back().type = ithoDeviceStatus::is_byte;
        ithoStatus.back().length = 1;
      }
      if (i2cbuf[6 + i] == 0x0F) {
        ithoStatus.back().type = ithoDeviceStatus::is_float;
        ithoStatus.back().length = 1;
        ithoStatus.back().divider = 2;
      }
      if (i2cbuf[6 + i] == 0x5B) {
        ithoStatus.back().type = ithoDeviceStatus::is_uint;
        ithoStatus.back().length = 2;
      }

    }

  }
  else {
    if (updateweb) {
      updateweb = false;
      jsonSysmessage("ithostatusformat", "failed");
    }
  }


#endif


}

void sendQueryStatus(bool & updateweb) {


  uint8_t command[] = { 0x82, 0x80, 0x24, 0x01, 0x04, 0x00, 0xD5 };

  while (digitalRead(SCLPIN) == LOW ) {
    yield();
    delay(1);
  }

#if defined (HW_VERSION_ONE)

#endif

#if defined (HW_VERSION_TWO)
  i2c_sendBytes(command, sizeof(command));

  uint8_t i2cbuf[512] {};
  size_t len = i2c_slave_receive(i2cbuf);

  if (len > 1 && i2cbuf[len - 1] == checksum(i2cbuf, len - 1)) {

    if (updateweb) {
      updateweb = false;
      jsonSysmessage("ithostatus", i2cbuf2string(i2cbuf, len).c_str());
    }

    int statusPos = 6; //first byte with status info

    if (!ithoStatus.empty()) {
      for (auto& ithoStat : ithoStatus) {
        //for (auto& ithoStat : ithoStatus) {
        auto tempVal = 0;
        for (int i = ithoStat.length; i > 0; i--) {
          tempVal |= i2cbuf[statusPos + (ithoStat.length - i)] << ((i - 1) * 8);
        }

        if (ithoStat.type == ithoDeviceStatus::is_byte) {
          ithoStat.value.byteval = (byte)tempVal;
        }
        if (ithoStat.type == ithoDeviceStatus::is_uint) {
          ithoStat.value.uintval = tempVal;
        }
        if (ithoStat.type == ithoDeviceStatus::is_int) {
          if (ithoStat.length == 4) {
            tempVal = (int32_t)tempVal;
          }
          if (ithoStat.length == 2) {
            tempVal = (int16_t)tempVal;
          }
          if (ithoStat.length == 1) {
            tempVal = (int8_t)tempVal;
          }
          ithoStat.value.intval = tempVal;
        }
        if (ithoStat.type == ithoDeviceStatus::is_float) {
          ithoStat.value.floatval = (float)tempVal / ithoStat.divider;
        }

        statusPos += ithoStat.length;

        if (strcmp(ithoStat.name.c_str(), "CO2") == 0) {
          if (ithoStat.value.intval == 0x8200) {
            ithoStat.type = ithoDeviceStatus::is_string;
            ithoStat.value.stringval = fanSensorErrors.begin()->second;
          }
        }
        if (strcmp(ithoStat.name.c_str(), "hRFspeedLevel") == 0) {
          if (ithoStat.value.intval == 0xEF) {
            ithoStat.type = ithoDeviceStatus::is_string;
            ithoStat.value.stringval = fanSensorErrors.begin()->second;
          }
        }
        if (strcmp(ithoStat.name.c_str(), "RelativeHumidity") == 0 || strcmp(ithoStat.name.c_str(), "humidity") == 0) {
          if (ithoStat.value.floatval > 1.0 && ithoStat.value.floatval < 100.0) {
            if (!systemConfig.syssht30) {
              ithoHum = ithoStat.value.floatval;
              itho_internal_hum_temp = true;
            }
          }
          else {
            ithoStat.type = ithoDeviceStatus::is_string;
            ithoStat.value.stringval = fanSensorErrors.begin()->second;
          }
        }
        if (strcmp(ithoStat.name.c_str(), "Temperature") == 0 || strcmp(ithoStat.name.c_str(), "temperature") == 0) {
          if (ithoStat.value.floatval > 1.0 && ithoStat.value.floatval < 100.0) {
            if (!systemConfig.syssht30) {
              ithoTemp = ithoStat.value.floatval;
              itho_internal_hum_temp = true;
            }
          }
          else {
            ithoStat.type = ithoDeviceStatus::is_string;
            ithoStat.value.stringval = fanSensorErrors.begin()->second;
          }
        }


      }
    }


  }
  else {
    if (updateweb) {
      updateweb = false;
      jsonSysmessage("ithostatus", "failed");
    }
  }

#endif

}

void sendQuery31DA(bool & updateweb) {

  uint8_t command[] = { 0x82, 0x80, 0x31, 0xDA, 0x04, 0x00, 0xEF };

  while (digitalRead(SCLPIN) == LOW ) {
    yield();
    delay(1);
  }

#if defined (HW_VERSION_ONE)

#endif


#if defined (HW_VERSION_TWO)
  i2c_sendBytes(command, sizeof(command));

  uint8_t i2cbuf[512] {};
  size_t len = i2c_slave_receive(i2cbuf);

  if (len > 1 && i2cbuf[len - 1] == checksum(i2cbuf, len - 1)) {

    if (updateweb) {
      updateweb = false;
      jsonSysmessage("itho31DA", i2cbuf2string(i2cbuf, len).c_str());
    }

    auto dataLength = i2cbuf[5];

    auto dataStart = 6;
    if (!ithoMeasurements.empty()) {
      ithoMeasurements.clear();
    }

    if (dataLength > 0) {
      ithoMeasurements.push_back(ithoDeviceMeasurements());
      ithoMeasurements.back().name.assign("AirQuality (%)");
      if (i2cbuf[0 + dataStart] > 200) {
        ithoMeasurements.back().type = ithoDeviceMeasurements::is_string;
        auto it = fanSensorErrors.find(i2cbuf[0 + dataStart]);
        if (it != fanSensorErrors.end()) ithoMeasurements.back().value.stringval = it->second;
        else ithoMeasurements.back().value.stringval = fanSensorErrors.rbegin()->second;
      }
      else {
        ithoMeasurements.back().type = ithoDeviceMeasurements::is_int;
        ithoMeasurements.back().value.intval = i2cbuf[0 + dataStart];
      }

      ithoMeasurements.push_back(ithoDeviceMeasurements());
      ithoMeasurements.back().name.assign("AirQbased on");
      ithoMeasurements.back().type = ithoDeviceMeasurements::is_int;
      ithoMeasurements.back().value.intval = i2cbuf[1 + dataStart];
    }
    if (dataLength > 1) {
      ithoMeasurements.push_back(ithoDeviceMeasurements());
      ithoMeasurements.back().name.assign("CO2level (ppm)");
      if (i2cbuf[2 + dataStart] >= 0x7F) {
        ithoMeasurements.back().type = ithoDeviceMeasurements::is_string;
        auto it = fanSensorErrors2.find(i2cbuf[2 + dataStart]);
        if (it != fanSensorErrors2.end()) ithoMeasurements.back().value.stringval = it->second;
        else ithoMeasurements.back().value.stringval = fanSensorErrors2.rbegin()->second;
      }
      else {
        int32_t tempVal = i2cbuf[2 + dataStart] << 8;
        tempVal |= i2cbuf[3 + dataStart];
        ithoMeasurements.back().type = ithoDeviceMeasurements::is_int;
        ithoMeasurements.back().value.intval = tempVal;
      }
    }
    if (dataLength > 3) {
      ithoMeasurements.push_back(ithoDeviceMeasurements());
      ithoMeasurements.back().name.assign("Indoorhumidity (%)");
      if (i2cbuf[4 + dataStart] > 200) {
        ithoMeasurements.back().type = ithoDeviceMeasurements::is_string;
        auto it = fanSensorErrors.find(i2cbuf[4 + dataStart]);
        if (it != fanSensorErrors.end()) ithoMeasurements.back().value.stringval = it->second;
        else ithoMeasurements.back().value.stringval = fanSensorErrors.rbegin()->second;
      }
      else {
        ithoMeasurements.back().type = ithoDeviceMeasurements::is_int;
        ithoMeasurements.back().value.intval = i2cbuf[4 + dataStart];
      }
    }
    if (dataLength > 4) {
      ithoMeasurements.push_back(ithoDeviceMeasurements());
      ithoMeasurements.back().name.assign("Outdoorhumidity (%)");
      if (i2cbuf[5 + dataStart] > 200) {
        ithoMeasurements.back().type = ithoDeviceMeasurements::is_string;
        auto it = fanSensorErrors.find(i2cbuf[5 + dataStart]);
        if (it != fanSensorErrors.end()) ithoMeasurements.back().value.stringval = it->second;
        else ithoMeasurements.back().value.stringval = fanSensorErrors.rbegin()->second;
      }
      else {
        ithoMeasurements.back().type = ithoDeviceMeasurements::is_float;
        ithoMeasurements.back().value.floatval = i2cbuf[5 + dataStart] / 2;
      }
    }
    if (dataLength > 5) {
      ithoMeasurements.push_back(ithoDeviceMeasurements());
      ithoMeasurements.back().name.assign("Exhausttemp (°C)");
      if (i2cbuf[6 + dataStart] >= 0x7F) {
        ithoMeasurements.back().type = ithoDeviceMeasurements::is_string;
        auto it = fanSensorErrors2.find(i2cbuf[6 + dataStart]);
        if (it != fanSensorErrors2.end()) ithoMeasurements.back().value.stringval = it->second;
        else ithoMeasurements.back().value.stringval = fanSensorErrors2.rbegin()->second;
      }
      else {
        int32_t tempVal = i2cbuf[6 + dataStart] << 8;
        tempVal |= i2cbuf[7 + dataStart];
        ithoMeasurements.back().type = ithoDeviceMeasurements::is_float;
        ithoMeasurements.back().value.floatval = tempVal / 100.0;
      }
    }
    if (dataLength > 7) {
      ithoMeasurements.push_back(ithoDeviceMeasurements());
      ithoMeasurements.back().name.assign("SupplyTemp (°C)");
      if (i2cbuf[8 + dataStart] >= 0x7F) {
        ithoMeasurements.back().type = ithoDeviceMeasurements::is_string;
        auto it = fanSensorErrors2.find(i2cbuf[8 + dataStart]);
        if (it != fanSensorErrors2.end()) ithoMeasurements.back().value.stringval = it->second;
        else ithoMeasurements.back().value.stringval = fanSensorErrors2.rbegin()->second;
      }
      else {
        int32_t tempVal = i2cbuf[8 + dataStart] << 8;
        tempVal |= i2cbuf[9 + dataStart];
        ithoMeasurements.back().type = ithoDeviceMeasurements::is_float;
        ithoMeasurements.back().value.floatval = tempVal / 100.0;
      }
    }
    if (dataLength > 9) {
      ithoMeasurements.push_back(ithoDeviceMeasurements());
      ithoMeasurements.back().name.assign("IndoorTemp (°C)");
      if (i2cbuf[10 + dataStart] >= 0x7F) {
        ithoMeasurements.back().type = ithoDeviceMeasurements::is_string;
        auto it = fanSensorErrors2.find(i2cbuf[10 + dataStart]);
        if (it != fanSensorErrors2.end()) ithoMeasurements.back().value.stringval = it->second;
        else ithoMeasurements.back().value.stringval = fanSensorErrors2.rbegin()->second;
      }
      else {
        int32_t tempVal = i2cbuf[10 + dataStart] << 8;
        tempVal |= i2cbuf[11 + dataStart];
        ithoMeasurements.back().type = ithoDeviceMeasurements::is_float;
        ithoMeasurements.back().value.floatval = tempVal / 100.0;
      }
    }
    if (dataLength > 11) {
      ithoMeasurements.push_back(ithoDeviceMeasurements());
      ithoMeasurements.back().name.assign("OutdoorTemp (°C)");
      if (i2cbuf[12 + dataStart] >= 0x7F) {
        ithoMeasurements.back().type = ithoDeviceMeasurements::is_string;
        auto it = fanSensorErrors2.find(i2cbuf[12 + dataStart]);
        if (it != fanSensorErrors2.end()) ithoMeasurements.back().value.stringval = it->second;
        else ithoMeasurements.back().value.stringval = fanSensorErrors2.rbegin()->second;
      }
      else {
        int32_t tempVal = i2cbuf[12 + dataStart] << 8;
        tempVal |= i2cbuf[13 + dataStart];
        ithoMeasurements.back().type = ithoDeviceMeasurements::is_float;
        ithoMeasurements.back().value.floatval = tempVal / 100.0;
      }
    }
    if (dataLength > 13) {
      ithoMeasurements.push_back(ithoDeviceMeasurements());
      ithoMeasurements.back().name.assign("SpeedCap");
      ithoMeasurements.back().type = ithoDeviceMeasurements::is_int;
      int32_t tempVal = i2cbuf[14 + dataStart] << 8;
      tempVal |= i2cbuf[15 + dataStart];
      ithoMeasurements.back().value.intval = tempVal;
    }
    if (dataLength > 15) {
      ithoMeasurements.push_back(ithoDeviceMeasurements());
      ithoMeasurements.back().name.assign("BypassPos (%)");
      if (i2cbuf[16 + dataStart] > 200) {
        ithoMeasurements.back().type = ithoDeviceMeasurements::is_string;
        auto it = fanSensorErrors.find(i2cbuf[16 + dataStart]);
        if (it != fanSensorErrors.end()) ithoMeasurements.back().value.stringval = it->second;
        else ithoMeasurements.back().value.stringval = fanSensorErrors.rbegin()->second;
      }
      else {
        ithoMeasurements.back().type = ithoDeviceMeasurements::is_float;
        ithoMeasurements.back().value.floatval = i2cbuf[16 + dataStart] / 2;
      }
    }
    if (dataLength > 16) {
      ithoMeasurements.push_back(ithoDeviceMeasurements());
      ithoMeasurements.back().name.assign("FanInfo");
      ithoMeasurements.back().type = ithoDeviceMeasurements::is_string;
      auto it = fanInfo.find(i2cbuf[17 + dataStart]);
      if (it != fanInfo.end()) ithoMeasurements.back().value.stringval = it->second;
      else ithoMeasurements.back().value.stringval = fanInfo.rbegin()->second;
    }
    if (dataLength > 17) {
      ithoMeasurements.push_back(ithoDeviceMeasurements());
      ithoMeasurements.back().name.assign("ExhFanSpeed (%)");
      if (i2cbuf[18 + dataStart] > 200) {
        ithoMeasurements.back().type = ithoDeviceMeasurements::is_string;
        auto it = fanSensorErrors.find(i2cbuf[18 + dataStart]);
        if (it != fanSensorErrors.end()) ithoMeasurements.back().value.stringval = it->second;
        else ithoMeasurements.back().value.stringval = fanSensorErrors.rbegin()->second;

      }
      else {
        ithoMeasurements.back().type = ithoDeviceMeasurements::is_float;
        ithoMeasurements.back().value.floatval = i2cbuf[18 + dataStart] / 2;
      }
    }
    if (dataLength > 18) {
      ithoMeasurements.push_back(ithoDeviceMeasurements());
      ithoMeasurements.back().name.assign("InFanSpeed (%)");
      if (i2cbuf[19 + dataStart] > 200) {
        ithoMeasurements.back().type = ithoDeviceMeasurements::is_string;
        auto it = fanSensorErrors.find(i2cbuf[19 + dataStart]);
        if (it != fanSensorErrors.end()) ithoMeasurements.back().value.stringval = it->second;
        else ithoMeasurements.back().value.stringval = fanSensorErrors.rbegin()->second;
      }
      else {
        ithoMeasurements.back().type = ithoDeviceMeasurements::is_float;
        ithoMeasurements.back().value.floatval = i2cbuf[19 + dataStart] / 2;
      }
    }
    if (dataLength > 19) {
      ithoMeasurements.push_back(ithoDeviceMeasurements());
      ithoMeasurements.back().name.assign("RemainingTime (min)");
      int32_t tempVal = i2cbuf[20 + dataStart] << 8;
      tempVal |= i2cbuf[21 + dataStart];
      ithoMeasurements.back().type = ithoDeviceMeasurements::is_int;
      ithoMeasurements.back().value.intval = tempVal;
    }
    if (dataLength > 21) {
      ithoMeasurements.push_back(ithoDeviceMeasurements());
      ithoMeasurements.back().name.assign("PostHeat (%)");
      if (i2cbuf[22 + dataStart] > 200) {
        ithoMeasurements.back().type = ithoDeviceMeasurements::is_string;
        auto it = fanHeatErrors.find(i2cbuf[22 + dataStart]);
        if (it != fanHeatErrors.end()) ithoMeasurements.back().value.stringval = it->second;
        else ithoMeasurements.back().value.stringval = fanHeatErrors.rbegin()->second;
      }
      else {
        ithoMeasurements.back().type = ithoDeviceMeasurements::is_float;
        ithoMeasurements.back().value.floatval = i2cbuf[22 + dataStart] / 2;
      }
    }
    if (dataLength > 22) {
      ithoMeasurements.push_back(ithoDeviceMeasurements());
      ithoMeasurements.back().name.assign("PreHeat (%)");
      if (i2cbuf[23 + dataStart] > 200) {
        ithoMeasurements.back().type = ithoDeviceMeasurements::is_string;
        auto it = fanHeatErrors.find(i2cbuf[23 + dataStart]);
        if (it != fanHeatErrors.end()) ithoMeasurements.back().value.stringval = it->second;
        else ithoMeasurements.back().value.stringval = fanHeatErrors.rbegin()->second;
      }
      else {
        ithoMeasurements.back().type = ithoDeviceMeasurements::is_float;
        ithoMeasurements.back().value.floatval = i2cbuf[23 + dataStart] / 2;
      }
    }
    if (dataLength > 23) {
      ithoMeasurements.push_back(ithoDeviceMeasurements());
      ithoMeasurements.back().name.assign("InFlow (l sec)");
      if (i2cbuf[24 + dataStart] >= 0x7F) {
        ithoMeasurements.back().type = ithoDeviceMeasurements::is_string;
        auto it = fanSensorErrors2.find(i2cbuf[24 + dataStart]);
        if (it != fanSensorErrors2.end()) ithoMeasurements.back().value.stringval = it->second;
        else ithoMeasurements.back().value.stringval = fanSensorErrors2.rbegin()->second;
      }
      else {
        int32_t tempVal = i2cbuf[24 + dataStart] << 8;
        tempVal |= i2cbuf[25 + dataStart];
        ithoMeasurements.back().type = ithoDeviceMeasurements::is_float;
        ithoMeasurements.back().value.floatval = tempVal / 100.0;
      }
    }
    if (dataLength > 25) {
      ithoMeasurements.push_back(ithoDeviceMeasurements());
      ithoMeasurements.back().name.assign("ExhFlow (l sec)");

      if (i2cbuf[26 + dataStart] >= 0x7F) {
        ithoMeasurements.back().type = ithoDeviceMeasurements::is_string;
        auto it = fanSensorErrors2.find(i2cbuf[26 + dataStart]);
        if (it != fanSensorErrors2.end()) ithoMeasurements.back().value.stringval = it->second;
        else ithoMeasurements.back().value.stringval = fanSensorErrors2.rbegin()->second;
      }
      else {
        int32_t tempVal = i2cbuf[26 + dataStart] << 8;
        tempVal |= i2cbuf[27 + dataStart];
        ithoMeasurements.back().type = ithoDeviceMeasurements::is_float;
        ithoMeasurements.back().value.floatval = tempVal / 100.0;
      }
    }

  }
  else {
    if (updateweb) {
      updateweb = false;
      jsonSysmessage("itho31DA", "failed");
    }
  }

#endif

}

void sendQuery31D9(bool & updateweb) {

  uint8_t command[] = { 0x82, 0x80, 0x31, 0xD9, 0x04, 0x00, 0xF0 };

  while (digitalRead(SCLPIN) == LOW ) {
    yield();
    delay(1);
  }

#if defined (HW_VERSION_ONE)

#endif

#if defined (HW_VERSION_TWO)
  i2c_sendBytes(command, sizeof(command));

  uint8_t i2cbuf[512] {};
  size_t len = i2c_slave_receive(i2cbuf);
  if (len > 1 && i2cbuf[len - 1] == checksum(i2cbuf, len - 1)) {

    if (updateweb) {
      updateweb = false;
      jsonSysmessage("itho31D9", i2cbuf2string(i2cbuf, len).c_str());
    }

    //auto dataLength = i2cbuf[5];

    auto dataStart = 6;

    if (!ithoInternalMeasurements.empty()) {
      ithoInternalMeasurements.clear();
    }

    float tempVal = i2cbuf[1 + dataStart] / 2.0;
    ithoDeviceMeasurements sTemp = {"speed status", ithoDeviceMeasurements::is_float, {.floatval = tempVal} } ;
    ithoInternalMeasurements.push_back(sTemp);

    int status = 0;
    if (i2cbuf[0 + dataStart] == 0x80) {
      status = 1; //fault
    }
    else {
      status = 0; //no fault
    }
    ithoInternalMeasurements.push_back({"internal fault", ithoDeviceMeasurements::is_int, {.intval = status}});
    if (i2cbuf[0 + dataStart] == 0x40) {
      status = 1; //frost cycle active
    }
    else {
      status = 0; //frost cycle not active
    }
    ithoInternalMeasurements.push_back({"frost cycle", ithoDeviceMeasurements::is_int, {.intval = status}});
    if (i2cbuf[0 + dataStart] == 0x20) {
      status = 1; //filter dirty
    }
    else {
      status = 0; //filter clean
    }
    ithoInternalMeasurements.push_back({"filter dirty", ithoDeviceMeasurements::is_int, {.intval = status}});
    //    if (i2cbuf[0 + dataStart] == 0x10) {
    //      //unknown
    //    }
    //    if (i2cbuf[0 + dataStart] == 0x08) {
    //      //unknown
    //    }
    //    if (i2cbuf[0 + dataStart] == 0x04) {
    //      //unknown
    //    }
    //    if (i2cbuf[0 + dataStart] == 0x02) {
    //      //unknown
    //    }
    //    if (i2cbuf[0 + dataStart] == 0x01) {
    //      //unknown
    //    }


  }
  else {
    if (updateweb) {
      updateweb = false;
      jsonSysmessage("itho31D9", "failed");
    }
  }

#endif

}

int32_t * sendQuery2410(bool & updateweb) {

  static int32_t values[3];
  values[0] = 0;
  values[1] = 0;
  values[2] = 0;

  uint8_t command[] = { 0x82, 0x80, 0x24, 0x10, 0x04, 0x13, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0xFF };

  command[23] = index2410;
  command[sizeof(command) - 1] = checksum(command, sizeof(command) - 1);

  while (digitalRead(SCLPIN) == LOW ) {
    yield();
    delay(1);
  }

#if defined (HW_VERSION_ONE)

#endif

#if defined (HW_VERSION_TWO)
  i2c_sendBytes(command, sizeof(command));

  uint8_t i2cbuf[512] {};
  size_t len = i2c_slave_receive(i2cbuf);
  if (len > 1 && i2cbuf[len - 1] == checksum(i2cbuf, len - 1)) {

    uint8_t tempBuf[] = { i2cbuf[9], i2cbuf[8], i2cbuf[7], i2cbuf[6] };
    std::memcpy(&values[0], tempBuf, 4);

    uint8_t tempBuf2[] = { i2cbuf[13], i2cbuf[12], i2cbuf[11], i2cbuf[10] };
    std::memcpy(&values[1], tempBuf2, 4);

    uint8_t tempBuf3[] = { i2cbuf[17], i2cbuf[16], i2cbuf[15], i2cbuf[14] };
    std::memcpy(&values[2], tempBuf3, 4);

    if (updateweb) {
      updateweb = false;
      jsonSysmessage("itho2410", i2cbuf2string(i2cbuf, len).c_str());

      char tempbuffer[256];

      sprintf(tempbuffer, "%d", values[0]);
      jsonSysmessage("itho2410cur", tempbuffer);
      sprintf(tempbuffer, "%d", values[1]);
      jsonSysmessage("itho2410min", tempbuffer);
      sprintf(tempbuffer, "%d", values[2]);
      jsonSysmessage("itho2410max", tempbuffer);
    }


  }
  else {
    if (updateweb) {
      updateweb = false;
      jsonSysmessage("itho2410", "failed");
    }

  }


#endif

  return values;

}

void setSetting2410(bool & updateweb) {

  if (index2410 == 7 && value2410 == 1 && (ithoDeviceID == 0x14 || ithoDeviceID == 0x1B)) {
    logMessagejson(F("<br>!!Warning!! Command ignored!<br>Setting index 7 to value 1 will switch off I2C!"), WEBINTERFACE);
    return;
  }

  uint8_t command[] = { 0x82, 0x80, 0x24, 0x10, 0x06, 0x13, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0xFF };

  command[23] = index2410;

  command[9] = value2410 & 0xFF;
  command[8] = (value2410 >> 8) & 0xFF;
  command[7] = (value2410 >> 16) & 0xFF;
  command[6] = (value2410 >> 24) & 0xFF;

  command[sizeof(command) - 1] = checksum(command, sizeof(command) - 1);

  jsonSysmessage("itho2410set", i2cbuf2string(command, sizeof(command)).c_str());


  while (digitalRead(SCLPIN) == LOW ) {
    yield();
    delay(1);
  }

#if defined (HW_VERSION_ONE)

#endif

#if defined (HW_VERSION_TWO)
  i2c_sendBytes(command, sizeof(command));

  uint8_t i2cbuf[512] {};
  size_t len = i2c_slave_receive(i2cbuf);
  if (len > 1 && i2cbuf[len - 1] == checksum(i2cbuf, len - 1)) {
    if (updateweb) {
      updateweb = false;
      if (len > 2) {
        if (command[6] == i2cbuf[6] && command[7] == i2cbuf[7] && command[8] == i2cbuf[8] && command[9] == i2cbuf[9] && command[23] == i2cbuf[23]) {
          jsonSysmessage("itho2410setres", "confirmed");
        }
        else {
          jsonSysmessage("itho2410setres", "confirmation failed");
        }
      }
      else {
        jsonSysmessage("itho2410setres", "failed");
      }
    }
  }



#endif

}

int quick_pow10(int n) {
  static int pow10[10] = {
    1, 10, 100, 1000, 10000,
    100000, 1000000, 10000000, 100000000, 1000000000
  };
  return pow10[n];
}

std::string i2cbuf2string(const uint8_t* data, size_t len) {
  std::string s;
  s.reserve(len * 3 + 2);
  for (size_t i = 0; i < len; ++i) {
    if (i)
      s += ' ';
    s += toHex(data[i] >> 4);
    s += toHex(data[i] & 0xF);
  }
  return s;
}
