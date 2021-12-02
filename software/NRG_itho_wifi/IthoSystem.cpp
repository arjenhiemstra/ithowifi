
#include "SystemConfig.h"
#include "IthoSystem.h"
#include "devices/cve14.h"
#include "devices/cve1B.h"
#include "devices/hru350.h"
#include "devices/hrueco.h"
#include "devices/demandflow.h"
#include "devices/autotemp.h"
#include "devices/wpu.h"
#include "devices/error_info_labels.h"
#include "hardware.h"
#include "i2c_esp32.h"
#include "notifyClients.h"
#include "flashLog.h"

uint8_t ithoDeviceID = 0;
uint8_t itho_fwversion = 0;
volatile uint16_t ithoCurrentVal = 0;
uint8_t id0 = 0;
uint8_t id1 = 0;
uint8_t id2 = 0;
const struct ihtoDeviceType* ithoDeviceptr = nullptr;
int16_t ithoSettingsLength = 0;
int16_t ithoStatusLabelLength = 0;
std::vector<ithoDeviceStatus> ithoStatus;
std::vector<ithoDeviceMeasurements> ithoMeasurements;
std::vector<ithoDeviceMeasurements> ithoInternalMeasurements;
struct lastCommand lastCmd;
ithoSettings * ithoSettingsArray = nullptr;

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
  const uint16_t **settingsMapping;
  uint16_t versionsMapLen;
  const char **settingsDescriptions;
  const uint8_t **statusLabelMapping;
  uint8_t statusMapLen;
  const struct ithoLabels *settingsStatusLabels;
};


const struct ihtoDeviceType ithoDevices[] {
  { 0x01, "Air curtain",        nullptr, 0, nullptr, nullptr, 0, nullptr },
  { 0x03, "HRU ECO-fan",        ithoHRUecoFanSettingsMap, sizeof(ithoHRUecoFanSettingsMap) / sizeof(ithoHRUecoFanSettingsMap[0]), ithoHRUecoSettingsLabels, ithoHRUecoFanStatusMap, sizeof(ithoHRUecoFanStatusMap) / sizeof(ithoHRUecoFanStatusMap[0]), ithoHRUecoStatusLabels },
  { 0x08, "LoadBoiler",         nullptr, 0, nullptr, nullptr, 0, nullptr },
  { 0x0A, "GGBB",               nullptr, 0, nullptr, nullptr, 0, nullptr },
  { 0x0B, "DemandFlow",         ithoDemandFlowSettingsMap, sizeof(ithoDemandFlowSettingsMap) / sizeof(ithoDemandFlowSettingsMap[0]), ithoDemandFlowSettingsLabels, ithoDemandFlowStatusMap, sizeof(ithoDemandFlowStatusMap) / sizeof(ithoDemandFlowStatusMap[0]), ithoDemandFlowStatusLabels  },
  { 0x0C, "CO2 relay",          nullptr, 0, nullptr, nullptr, 0, nullptr },
  { 0x0D, "Heatpump",           ithoWPUSettingsMap, sizeof(ithoWPUSettingsMap) / sizeof(ithoWPUSettingsMap[0]), ithoWPUSettingsLabels, ithoWPUStatusMap, sizeof(ithoWPUStatusMap) / sizeof(ithoWPUStatusMap[0]), ithoWPUStatusLabels },
  { 0x0E, "OLB Single",         nullptr, 0, nullptr, nullptr, 0, nullptr },
  { 0x0F, "AutoTemp",           ithoAutoTempSettingsMap, sizeof(ithoAutoTempSettingsMap) / sizeof(ithoAutoTempSettingsMap[0]), ithoAutoTempSettingsLabels, ithoAutoTempStatusMap, sizeof(ithoAutoTempStatusMap) / sizeof(ithoAutoTempStatusMap[0]), ithoAutoTempStatusLabels },
  { 0x10, "OLB Double",         nullptr, 0, nullptr, nullptr, 0, nullptr },
  { 0x11, "RF+",                nullptr, 0, nullptr, nullptr, 0, nullptr },
  { 0x14, "CVE",                itho14SettingsMap, sizeof(itho14SettingsMap) / sizeof(itho14SettingsMap[0]), ithoCVE14SettingsLabels, itho14StatusMap, sizeof(itho14StatusMap) / sizeof(itho14StatusMap[0]), ithoCVE14StatusLabels  },
  { 0x15, "Extended",           nullptr, 0, nullptr, nullptr, 0, nullptr },
  { 0x16, "Extended Plus",      nullptr, 0, nullptr, nullptr, 0, nullptr },
  { 0x1A, "AreaFlow",           nullptr, 0, nullptr, nullptr, 0, nullptr },
  { 0x1B, "CVE-Silent",         itho1BSettingsMap, sizeof(itho1BSettingsMap) / sizeof(itho1BSettingsMap[0]), ithoCVE1BSettingsLabels, itho1BStatusMap, sizeof(itho1BStatusMap) / sizeof(itho1BStatusMap[0]), ithoCVE1BStatusLabels  },
  { 0x1C, "CVE-SilentExt",      nullptr, 0, nullptr, nullptr, 0, nullptr },
  { 0x1D, "CVE-SilentExtPlus",  nullptr, 0, nullptr, nullptr, 0, nullptr },
  { 0x20, "RF_CO2",             nullptr, 0, nullptr, nullptr, 0, nullptr },
  { 0x2B, "HRU 350",            ithoHRU350SettingsMap, sizeof(ithoHRU350SettingsMap) / sizeof(ithoHRU350SettingsMap[0]), ithoHRU350SettingsLabels, ithoHRU350StatusMap, sizeof(ithoHRU350StatusMap) / sizeof(ithoHRU350StatusMap[0]), ithoHRU350StatusLabels  }
};



const char* getIthoType(const uint8_t deviceID) {
  static char ithoDeviceType[32] = "Unkown device type";

  const struct ihtoDeviceType* ithoDevicesptr = ithoDevices;
  const struct ihtoDeviceType* ithoDevicesendPtr = ithoDevices + sizeof(ithoDevices) / sizeof(ithoDevices[0]);
  while ( ithoDevicesptr < ithoDevicesendPtr ) {
    if (ithoDevicesptr->ID == deviceID) {
      strcpy(ithoDeviceType, ithoDevicesptr->name);
    }
    ithoDevicesptr++;
  }
  return ithoDeviceType;
}

int getSettingsLength(const uint8_t deviceID, const uint8_t version) {

  const struct ihtoDeviceType* ithoDevicesptr = ithoDevices;
  const struct ihtoDeviceType* ithoDevicesendPtr = ithoDevices + sizeof(ithoDevices) / sizeof(ithoDevices[0]);
  while ( ithoDevicesptr < ithoDevicesendPtr ) {
    if (ithoDevicesptr->ID == deviceID) {
      if (ithoDevicesptr->settingsMapping == nullptr) {
        return -2; //Settings not available for this device
      }
      if (version > (ithoDevicesptr->versionsMapLen - 1)) {
        return -3; //Settings not available for this version
      }

      for (int i = 0; i < 999; i++) {
        if ((int) * (*(ithoDevicesptr->settingsMapping + version) + i) == 999) {
          //end of array
          if (ithoSettingsArray == nullptr) ithoSettingsArray = new ithoSettings[i];
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
    logMessagejson("Settings not available for this device or its firmware version", WEBINTERFACE);
    return;
  }

  StaticJsonDocument<512> doc;
  JsonObject root = doc.to<JsonObject>();

  root["Index"] = i;
  root["Description"] = settingsPtr->settingsDescriptions[(int) * (*(settingsPtr->settingsMapping + version) + i)];

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
      if (ithoSettingsArray[index2410].type == ithoSettings::is_int8) {
        root["Current"] = *(reinterpret_cast<int8_t*>(resultPtr2410 + 0));
        root["Minimum"] = *(reinterpret_cast<int8_t*>(resultPtr2410 + 1));
        root["Maximum"] = *(reinterpret_cast<int8_t*>(resultPtr2410 + 2));
      }
      else if (ithoSettingsArray[index2410].type == ithoSettings::is_int16) {
        root["Current"] = *(reinterpret_cast<int16_t*>(resultPtr2410 + 0));
        root["Minimum"] = *(reinterpret_cast<int16_t*>(resultPtr2410 + 1));
        root["Maximum"] = *(reinterpret_cast<int16_t*>(resultPtr2410 + 2));
      }
      else if (ithoSettingsArray[index2410].type == ithoSettings::is_uint8 || ithoSettingsArray[index2410].type == ithoSettings::is_uint16 || ithoSettingsArray[index2410].type == ithoSettings::is_uint32) {
        root["Current"] = *(reinterpret_cast<uint32_t*>(resultPtr2410 + 0));
        root["Minimum"] = *(reinterpret_cast<uint32_t*>(resultPtr2410 + 1));
        root["Maximum"] = *(reinterpret_cast<uint32_t*>(resultPtr2410 + 2));
      }
      else if (ithoSettingsArray[index2410].type == ithoSettings::is_float2) {
        root["Current"] = *(reinterpret_cast<int32_t*>(resultPtr2410 + 0)) / 2.0;
        root["Minimum"] = *(reinterpret_cast<int32_t*>(resultPtr2410 + 1)) / 2.0;
        root["Maximum"] = *(reinterpret_cast<int32_t*>(resultPtr2410 + 2)) / 2.0;
      }
      else if (ithoSettingsArray[index2410].type == ithoSettings::is_float10) {
        root["Current"] = *(reinterpret_cast<int32_t*>(resultPtr2410 + 0)) / 10.0;
        root["Minimum"] = *(reinterpret_cast<int32_t*>(resultPtr2410 + 1)) / 10.0;
        root["Maximum"] = *(reinterpret_cast<int32_t*>(resultPtr2410 + 2)) / 10.0;
      }
      else if (ithoSettingsArray[index2410].type == ithoSettings::is_float100) {
        root["Current"] = *(reinterpret_cast<int32_t*>(resultPtr2410 + 0)) / 100.0;
        root["Minimum"] = *(reinterpret_cast<int32_t*>(resultPtr2410 + 1)) / 100.0;
        root["Maximum"] = *(reinterpret_cast<int32_t*>(resultPtr2410 + 2)) / 100.0;
      }
      else if (ithoSettingsArray[index2410].type == ithoSettings::is_float1000) {
        root["Current"] = *(reinterpret_cast<int32_t*>(resultPtr2410 + 0)) / 1000.0;
        root["Minimum"] = *(reinterpret_cast<int32_t*>(resultPtr2410 + 1)) / 1000.0;
        root["Maximum"] = *(reinterpret_cast<int32_t*>(resultPtr2410 + 2)) / 1000.0;
      }
      else {
        root["Current"] = *(resultPtr2410 + 0);
        root["Minimum"] = *(resultPtr2410 + 1);
        root["Maximum"] = *(resultPtr2410 + 2);
      }
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

int getStatusLabelLength(const uint8_t deviceID, const uint8_t version) {

  const struct ihtoDeviceType* ithoDevicesptr = ithoDevices;
  const struct ihtoDeviceType* ithoDevicesendPtr = ithoDevices + sizeof(ithoDevices) / sizeof(ithoDevices[0]);
  while ( ithoDevicesptr < ithoDevicesendPtr ) {
    if (ithoDevicesptr->ID == deviceID) {
      if (ithoDevicesptr->statusLabelMapping == nullptr) {
        return -2; //Labels not available for this device
      }
      if (version > (ithoDevicesptr->statusMapLen - 1)) {
        return -3; //Labels not available for this version
      }

      for (int i = 0; i < 255; i++) {
        if ((int) * (*(ithoDevicesptr->statusLabelMapping + version) + i) == 255) {
          //end of array
          return i;
        }
      }
    }
    ithoDevicesptr++;
  }
  return -1;
}

const char* getSatusLabel(const uint8_t i, const struct ihtoDeviceType* statusPtr, const uint8_t version) {

  if (statusPtr == nullptr) {
    if (systemConfig.api_normalize == 0) {
      return ithoLabelErrors[0].labelFull;
    }
    else {
      return ithoLabelErrors[0].labelNormalized;
    }    
  }
  else if (ithoStatusLabelLength == -2) {
    if (systemConfig.api_normalize == 0) {
      return ithoLabelErrors[1].labelFull;
    }
    else {
      return ithoLabelErrors[1].labelNormalized;
    }
  }
  else if (ithoStatusLabelLength == -3) {
    if (systemConfig.api_normalize == 0) {
      return ithoLabelErrors[2].labelFull;
    }
    else {
      return ithoLabelErrors[2].labelNormalized;
    }        
  }
  else if (!(i < ithoStatusLabelLength)) {
    if (systemConfig.api_normalize == 0) {
      return ithoLabelErrors[3].labelFull;
    }
    else {
      return ithoLabelErrors[3].labelNormalized;
    }    
  }
  else {
    if (systemConfig.api_normalize == 0) {
      return (statusPtr->settingsStatusLabels[(int) * (*(statusPtr->statusLabelMapping + version) + i)].labelFull);
    }
    else {
      return (statusPtr->settingsStatusLabels[(int) * (*(statusPtr->statusLabelMapping + version) + i)].labelNormalized);
    }
  }

}

void updateSetting(const uint8_t i, const int32_t value, bool webupdate) {

  if (xSemaphoreTake(mutexI2Ctask, (TickType_t) 500 / portTICK_PERIOD_MS) == pdTRUE) {

    i2c_result_updateweb = webupdate;
    index2410 = i;
    value2410 = value;
    set2410 = true;
  }
}

const struct ihtoDeviceType* getDevicePtr(const uint8_t deviceID) {

  const struct ihtoDeviceType* ithoDevicesptr = ithoDevices;
  const struct ihtoDeviceType* ithoDevicesendPtr = ithoDevices + sizeof(ithoDevices) / sizeof(ithoDevices[0]);
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

  i2c_sendBytes(command, sizeof(command));

}

uint8_t cmdCounter = 0;

void sendButton(const uint8_t number, bool & updateweb) {

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

  i2c_sendBytes(command, sizeof(command));

  buttonResult = true;

}

void sendTimer(const uint8_t timer, bool & updateweb) {

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

  i2c_sendBytes(command, sizeof(command));

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

  i2c_sendBytes(command, sizeof(command));

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

  i2c_sendBytes(command, sizeof(command));

}

void sendQueryDevicetype(bool & updateweb) {

  uint8_t command[] = { 0x82, 0x80, 0x90, 0xE0, 0x04, 0x00, 0x8A };

  while (digitalRead(SCLPIN) == LOW ) {
    yield();
    delay(1);
  }

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

}

void sendQueryStatusFormat(bool & updateweb) {

  uint8_t command[] = { 0x82, 0x80, 0x24, 0x00, 0x04, 0x00, 0xD6 };

  while (digitalRead(SCLPIN) == LOW ) {
    yield();
    delay(1);
  }

  i2c_sendBytes(command, sizeof(command));

  uint8_t i2cbuf[512] {};
  uint8_t len = i2c_slave_receive(i2cbuf);
  if (len > 1 && i2cbuf[len - 1] == checksum(i2cbuf, len - 1)) {
    if (updateweb) {
      updateweb = false;
      jsonSysmessage("ithostatusformat", i2cbuf2string(i2cbuf, len).c_str());
    }

    if (!ithoStatus.empty()) {
      ithoStatus.clear();
    }
    if (!(itho_fwversion > 0)) return;
    ithoStatusLabelLength = getStatusLabelLength(ithoDeviceID, itho_fwversion);
    const uint8_t endPos = i2cbuf[5];
    for (uint8_t i = 0; i < endPos; i++) {
      ithoStatus.push_back(ithoDeviceStatus());

//      char fStringBuf[32];
//      getSatusLabel(i, ithoDeviceptr, itho_fwversion, fStringBuf);

      

      ithoStatus.back().divider = 0;
      if ((i2cbuf[6 + i] & 0x07) == 0) { //integer value
        if ((i2cbuf[6 + i] & 0x80) == 0) { //unsigned value
          ithoStatus.back().type = ithoDeviceStatus::is_uint;
        }
        else {
          ithoStatus.back().type = ithoDeviceStatus::is_int;
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


}

void sendQueryStatus(bool & updateweb) {


  uint8_t command[] = { 0x82, 0x80, 0x24, 0x01, 0x04, 0x00, 0xD5 };

  while (digitalRead(SCLPIN) == LOW ) {
    yield();
    delay(1);
  }

  i2c_sendBytes(command, sizeof(command));

  uint8_t i2cbuf[512] {};
  size_t len = i2c_slave_receive(i2cbuf);

  if (len > 1 && i2cbuf[len - 1] == checksum(i2cbuf, len - 1)) {

    if (updateweb) {
      updateweb = false;
      jsonSysmessage("ithostatus", i2cbuf2string(i2cbuf, len).c_str());
    }

    int statusPos = 6; //first byte with status info
    int labelPos = 0;
    if (!ithoStatus.empty()) {
      for (auto& ithoStat : ithoStatus) {
        ithoStat.name = getSatusLabel(labelPos, ithoDeviceptr, itho_fwversion);
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

        if (strcmp("Highest CO2 concentration (ppm)", ithoStat.name) == 0 || strcmp("highest-co2-concentration_ppm", ithoStat.name) == 0) {
          if (ithoStat.value.intval == 0x8200) {
            ithoStat.type = ithoDeviceStatus::is_string;
            ithoStat.value.stringval = fanSensorErrors.begin()->second;
          }
        }
        if (strcmp("Highest RH concentration (%)", ithoStat.name) == 0 || strcmp("highest-rh-concentration_perc", ithoStat.name) == 0) {
          if (ithoStat.value.intval == 0xEF) {
            ithoStat.type = ithoDeviceStatus::is_string;
            ithoStat.value.stringval = fanSensorErrors.begin()->second;
          }
        }
        if (strcmp("RelativeHumidity", ithoStat.name) == 0 || strcmp("relativehumidity", ithoStat.name) == 0) {
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
        if (strcmp("Temperature", ithoStat.name) == 0 || strcmp("temperature", ithoStat.name) == 0) {
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

      labelPos++;
      }
    }


  }
  else {
    if (updateweb) {
      updateweb = false;
      jsonSysmessage("ithostatus", "failed");
    }
  }

}

void sendQuery31DA(bool & updateweb) {

  uint8_t command[] = { 0x82, 0x80, 0x31, 0xDA, 0x04, 0x00, 0xEF };

  while (digitalRead(SCLPIN) == LOW ) {
    yield();
    delay(1);
  }

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
    const int labelLen = 19;
    static const char* labels31DA[labelLen] {};
    for (int i = 0; i < labelLen; i++) {
      if (systemConfig.api_normalize == 0) {
        labels31DA[i] = itho31DALabels[i].labelFull;
      }
      else {
        labels31DA[i] = itho31DALabels[i].labelNormalized;
      }
    }
    if (dataLength > 0) {
      ithoMeasurements.push_back(ithoDeviceMeasurements());
      ithoMeasurements.back().name = labels31DA[0];
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
      ithoMeasurements.back().name = labels31DA[1];
      ithoMeasurements.back().type = ithoDeviceMeasurements::is_int;
      ithoMeasurements.back().value.intval = i2cbuf[1 + dataStart];
    }
    if (dataLength > 1) {
      ithoMeasurements.push_back(ithoDeviceMeasurements());
      ithoMeasurements.back().name = labels31DA[2];
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
      ithoMeasurements.back().name = labels31DA[3];
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
      ithoMeasurements.back().name = labels31DA[4];
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
      ithoMeasurements.back().name = labels31DA[5];
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
      ithoMeasurements.back().name = labels31DA[6];
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
      ithoMeasurements.back().name = labels31DA[7];
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
      ithoMeasurements.back().name = labels31DA[8];
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
      ithoMeasurements.back().name = labels31DA[9];
      ithoMeasurements.back().type = ithoDeviceMeasurements::is_int;
      int32_t tempVal = i2cbuf[14 + dataStart] << 8;
      tempVal |= i2cbuf[15 + dataStart];
      ithoMeasurements.back().value.intval = tempVal;
    }
    if (dataLength > 15) {
      ithoMeasurements.push_back(ithoDeviceMeasurements());
      ithoMeasurements.back().name = labels31DA[10];
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
      ithoMeasurements.back().name = labels31DA[11];
      ithoMeasurements.back().type = ithoDeviceMeasurements::is_string;
      auto it = fanInfo.find(i2cbuf[17 + dataStart] & 0x1F);
      if (it != fanInfo.end()) ithoMeasurements.back().value.stringval = it->second;
      else ithoMeasurements.back().value.stringval = fanInfo.rbegin()->second;
    }
    if (dataLength > 17) {
      ithoMeasurements.push_back(ithoDeviceMeasurements());
      ithoMeasurements.back().name = labels31DA[12];
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
      ithoMeasurements.back().name = labels31DA[13];
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
      ithoMeasurements.back().name = labels31DA[14];
      int32_t tempVal = i2cbuf[20 + dataStart] << 8;
      tempVal |= i2cbuf[21 + dataStart];
      ithoMeasurements.back().type = ithoDeviceMeasurements::is_int;
      ithoMeasurements.back().value.intval = tempVal;
    }
    if (dataLength > 21) {
      ithoMeasurements.push_back(ithoDeviceMeasurements());
      ithoMeasurements.back().name = labels31DA[15];
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
      ithoMeasurements.back().name = labels31DA[16];
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
      ithoMeasurements.back().name = labels31DA[17];
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
      ithoMeasurements.back().name = labels31DA[18];

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

}

void sendQuery31D9(bool & updateweb) {

  uint8_t command[] = { 0x82, 0x80, 0x31, 0xD9, 0x04, 0x00, 0xF0 };

  while (digitalRead(SCLPIN) == LOW ) {
    yield();
    delay(1);
  }

  i2c_sendBytes(command, sizeof(command));

  uint8_t i2cbuf[512] {};
  size_t len = i2c_slave_receive(i2cbuf);
  if (len > 1 && i2cbuf[len - 1] == checksum(i2cbuf, len - 1)) {

    if (updateweb) {
      updateweb = false;
      jsonSysmessage("itho31D9", i2cbuf2string(i2cbuf, len).c_str());
    }

    auto dataStart = 6;

    if (!ithoInternalMeasurements.empty()) {
      ithoInternalMeasurements.clear();
    }
    const int labelLen = 4;
    static const char* labels31D9[labelLen] {};
    for (int i = 0; i < labelLen; i++) {
      if (systemConfig.api_normalize == 0) {
        labels31D9[i] = itho31D9Labels[i].labelFull;
      }
      else {
        labels31D9[i] = itho31D9Labels[i].labelNormalized;
      }
    }

    float tempVal = i2cbuf[1 + dataStart] / 2.0;
    ithoDeviceMeasurements sTemp = {labels31D9[0], ithoDeviceMeasurements::is_float, {.floatval = tempVal} } ;
    ithoInternalMeasurements.push_back(sTemp);

    int status = 0;
    if (i2cbuf[0 + dataStart] == 0x80) {
      status = 1; //fault
    }
    else {
      status = 0; //no fault
    }
    ithoInternalMeasurements.push_back({labels31D9[1], ithoDeviceMeasurements::is_int, {.intval = status}});
    if (i2cbuf[0 + dataStart] == 0x40) {
      status = 1; //frost cycle active
    }
    else {
      status = 0; //frost cycle not active
    }
    ithoInternalMeasurements.push_back({labels31D9[2], ithoDeviceMeasurements::is_int, {.intval = status}});
    if (i2cbuf[0 + dataStart] == 0x20) {
      status = 1; //filter dirty
    }
    else {
      status = 0; //filter clean
    }
    ithoInternalMeasurements.push_back({labels31D9[3], ithoDeviceMeasurements::is_int, {.intval = status}});
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


    ithoSettingsArray[index2410].value = values[0];

    if (((i2cbuf[22] >> 3) & 0x07) == 0) {
      ithoSettingsArray[index2410].length = 1;
    }
    else {
      ithoSettingsArray[index2410].length = (i2cbuf[22] >> 3) & 0x07;
    }

    if ((i2cbuf[22] & 0x07) == 0) { //integer value
      if ((i2cbuf[22] & 0x80) == 0) { //unsigned value
        if (ithoSettingsArray[index2410].length == 1) {
          ithoSettingsArray[index2410].type = ithoSettings::is_uint8;
        }
        else if (ithoSettingsArray[index2410].length == 2) {
          ithoSettingsArray[index2410].type = ithoSettings::is_uint16;
        }
        else {
          ithoSettingsArray[index2410].type = ithoSettings::is_uint32;
        }
      }
      else {
        if (ithoSettingsArray[index2410].length == 1) {
          ithoSettingsArray[index2410].type = ithoSettings::is_int8;
        }
        else if (ithoSettingsArray[index2410].length == 2) {
          ithoSettingsArray[index2410].type = ithoSettings::is_int16;
        }
        else {
          ithoSettingsArray[index2410].type = ithoSettings::is_int32;
        }
      }
    }
    else { //float

      if ((i2cbuf[22] & 0x04) != 0) {
        ithoSettingsArray[index2410].type = ithoSettings::is_float1000;
      }
      else if ((i2cbuf[22] & 0x02) != 0) {
        ithoSettingsArray[index2410].type = ithoSettings::is_float100;
      }
      else if ((i2cbuf[22] & 0x01) != 0) {
        ithoSettingsArray[index2410].type = ithoSettings::is_float10;
      }
      else {
        ithoSettingsArray[index2410].type = ithoSettings::is_unknown;
      }

    }
    //special cases
    if (i2cbuf[22] == 0x01) {
      ithoSettingsArray[index2410].type = ithoSettings::is_uint8;
    }

    if (updateweb) {
      updateweb = false;
      jsonSysmessage("itho2410", i2cbuf2string(i2cbuf, len).c_str());

      char tempbuffer0[256];
      char tempbuffer1[256];
      char tempbuffer2[256];

      if (ithoSettingsArray[index2410].type == ithoSettings::is_uint8 || ithoSettingsArray[index2410].type == ithoSettings::is_uint16 || ithoSettingsArray[index2410].type == ithoSettings::is_uint32) { //unsigned value
        uint32_t val[3];
        std::memcpy(&val[0], &values[0], sizeof(val[0]));
        std::memcpy(&val[1], &values[1], sizeof(val[0]));
        std::memcpy(&val[2], &values[2], sizeof(val[0]));
        sprintf(tempbuffer0, "%u", val[0]);
        sprintf(tempbuffer1, "%u", val[1]);
        sprintf(tempbuffer2, "%u", val[2]);
      }
      else if (ithoSettingsArray[index2410].type == ithoSettings::is_int8) {
        int8_t val[3];
        std::memcpy(&val[0], &values[0], sizeof(val[0]));
        std::memcpy(&val[1], &values[1], sizeof(val[0]));
        std::memcpy(&val[2], &values[2], sizeof(val[0]));
        sprintf(tempbuffer0, "%d", val[0]);
        sprintf(tempbuffer1, "%d", val[1]);
        sprintf(tempbuffer2, "%d", val[2]);
      }
      else if (ithoSettingsArray[index2410].type == ithoSettings::is_int16) {
        int16_t val[3];
        std::memcpy(&val[0], &values[0], sizeof(val[0]));
        std::memcpy(&val[1], &values[1], sizeof(val[0]));
        std::memcpy(&val[2], &values[2], sizeof(val[0]));
        sprintf(tempbuffer0, "%d", val[0]);
        sprintf(tempbuffer1, "%d", val[1]);
        sprintf(tempbuffer2, "%d", val[2]);
      }
      else if (ithoSettingsArray[index2410].type == ithoSettings::is_int32) {
        sprintf(tempbuffer0, "%d", values[0]);
        sprintf(tempbuffer1, "%d", values[1]);
        sprintf(tempbuffer2, "%d", values[2]);
      }
      else if (ithoSettingsArray[index2410].type == ithoSettings::is_float10) {
        float val[3];
        val[0] = values[0] / 10.0f;
        val[1] = values[1] / 10.0f;
        val[2] = values[2] / 10.0f;
        sprintf(tempbuffer0, "%.1f", val[0]);
        sprintf(tempbuffer1, "%.1f", val[1]);
        sprintf(tempbuffer2, "%.1f", val[2]);
      }
      else if (ithoSettingsArray[index2410].type == ithoSettings::is_float100) {
        float val[3];
        val[0] = values[0] / 100.0f;
        val[1] = values[1] / 100.0f;
        val[2] = values[2] / 100.0f;
        sprintf(tempbuffer0, "%.2f", val[0]);
        sprintf(tempbuffer1, "%.2f", val[1]);
        sprintf(tempbuffer2, "%.2f", val[2]);
      }
      else if (ithoSettingsArray[index2410].type == ithoSettings::is_float1000) {
        float val[3];
        val[0] = values[0] / 1000.0f;
        val[1] = values[1] / 1000.0f;
        val[2] = values[2] / 1000.0f;
        sprintf(tempbuffer0, "%.3f", val[0]);
        sprintf(tempbuffer1, "%.3f", val[1]);
        sprintf(tempbuffer2, "%.3f", val[2]);
      }
      else {
        sprintf(tempbuffer0, "%d", values[0]);
        sprintf(tempbuffer1, "%d", values[1]);
        sprintf(tempbuffer2, "%d", values[2]);
      }
      jsonSysmessage("itho2410cur", tempbuffer0);
      jsonSysmessage("itho2410min", tempbuffer1);
      jsonSysmessage("itho2410max", tempbuffer2);

    }


  }
  else {
    if (updateweb) {
      updateweb = false;
      jsonSysmessage("itho2410", "failed");
    }

  }

  return values;

}

void setSetting2410(bool & updateweb) {

  if (index2410 == 7 && value2410 == 1 && (ithoDeviceID == 0x14 || ithoDeviceID == 0x1B)) {
    logMessagejson("<br>!!Warning!! Command ignored!<br>Setting index 7 to value 1 will switch off I2C!", WEBINTERFACE);
    return;
  }

  uint8_t command[] = { 0x82, 0x80, 0x24, 0x10, 0x06, 0x13, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0xFF };

  command[23] = index2410;

  if (ithoSettingsArray[index2410].type == ithoSettings::is_uint8 || ithoSettingsArray[index2410].type == ithoSettings::is_int8) {
    command[9] = value2410 & 0xFF;
  }
  else if (ithoSettingsArray[index2410].type == ithoSettings::is_uint16 || ithoSettingsArray[index2410].type == ithoSettings::is_int16) {
    command[9] = value2410 & 0xFF;
    command[8] = (value2410 >> 8) & 0xFF;
  }
  else if (ithoSettingsArray[index2410].type == ithoSettings::is_uint32 || ithoSettingsArray[index2410].type == ithoSettings::is_int32 || ithoSettingsArray[index2410].type == ithoSettings::is_float2 || ithoSettingsArray[index2410].type == ithoSettings::is_float10 || ithoSettingsArray[index2410].type == ithoSettings::is_float100 || ithoSettingsArray[index2410].type == ithoSettings::is_float1000) {
    command[9] = value2410 & 0xFF;
    command[8] = (value2410 >> 8) & 0xFF;
    command[7] = (value2410 >> 16) & 0xFF;
    command[6] = (value2410 >> 24) & 0xFF;
  }
  else {
    if (updateweb) {
      updateweb = false;
      jsonSysmessage("itho2410setres", "format error, first use query 2410");
    }
    return; //unsupported value format
  }

  command[sizeof(command) - 1] = checksum(command, sizeof(command) - 1);

  jsonSysmessage("itho2410set", i2cbuf2string(command, sizeof(command)).c_str());


  while (digitalRead(SCLPIN) == LOW ) {
    yield();
    delay(1);
  }

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
