
#include "IthoSystem.h"

#include "devices/cve14.h"
#include "devices/cve1B.h"
#include "devices/hru200.h"
#include "devices/hru250_300.h"
#include "devices/hru350.h"
#include "devices/hrueco.h"
#include "devices/demandflow.h"
#include "devices/autotemp.h"
#include "devices/wpu.h"
#include "devices/error_info_labels.h"

uint8_t ithoDeviceGroup = 0;
uint8_t ithoDeviceID = 0;
uint8_t itho_hwversion = 0;
uint8_t itho_fwversion = 0;
volatile uint16_t ithoCurrentVal = 0;
const struct ihtoDeviceType *ithoDeviceptr = nullptr;
int16_t ithoSettingsLength = 0;
int16_t ithoStatusLabelLength = 0;
std::vector<ithoDeviceStatus> ithoStatus;
std::vector<ithoDeviceMeasurements> ithoMeasurements;
std::vector<ithoDeviceMeasurements> ithoInternalMeasurements;
std::vector<ithoDeviceMeasurements> ithoCounters;

struct lastCommand lastCmd;
ithoSettings *ithoSettingsArray = nullptr;

const std::map<cmdOrigin, const char *> cmdOriginMap = {
    {cmdOrigin::HTMLAPI, "HTML API"},
    {cmdOrigin::MQTTAPI, "MQTT API"},
    {cmdOrigin::REMOTE, "remote"},
    {cmdOrigin::WEB, "web interface"},
    {cmdOrigin::UNKNOWN, "unknown"}};

// bool get2410 = false;
//  bool set2410 = false;
//  uint8_t index2410 = 0;
//  int32_t value2410 = 0;
int32_t *resultPtr2410 = nullptr;
bool i2c_result_updateweb = false;
bool i2c_31d9_done = false;

bool itho_internal_hum_temp = false;
float ithoHum = 0;
float ithoTemp = 0;

int currentIthoDeviceGroup() { return ithoDeviceGroup; }
int currentIthoDeviceID() { return ithoDeviceID; }
uint8_t currentItho_hwversion() { return itho_hwversion; }
uint8_t currentItho_fwversion() { return itho_fwversion; }
uint16_t currentIthoSettingsLength() { return ithoSettingsLength; }
int16_t currentIthoStatusLabelLength() { return ithoStatusLabelLength; }

struct retryItem
{
  int index;
  int retries;
};

std::vector<retryItem> retryList;

struct ihtoDeviceType
{
  uint8_t DG; // device group?
  uint8_t ID;
  const char *name;
  const uint16_t **settingsMapping;
  uint16_t versionsMapLen;
  const char **settingsDescriptions;
  const uint8_t **statusLabelMapping;
  uint8_t statusMapLen;
  const struct ithoLabels *settingsStatusLabels;
};

const struct ihtoDeviceType ithoDevices[]{
    {0x00, 0x01, "Air curtain", nullptr, 0, nullptr, nullptr, 0, nullptr},
    {0x00, 0x03, "HRU ECO-fan", ithoHRUecoFanSettingsMap, sizeof(ithoHRUecoFanSettingsMap) / sizeof(ithoHRUecoFanSettingsMap[0]), ithoHRUecoSettingsLabels, ithoHRUecoFanStatusMap, sizeof(ithoHRUecoFanStatusMap) / sizeof(ithoHRUecoFanStatusMap[0]), ithoHRUecoStatusLabels},
    {0x00, 0x08, "LoadBoiler", nullptr, 0, nullptr, nullptr, 0, nullptr},
    {0x00, 0x0A, "GGBB", nullptr, 0, nullptr, nullptr, 0, nullptr},
    {0x00, 0x0B, "DemandFlow", ithoDemandFlowSettingsMap, sizeof(ithoDemandFlowSettingsMap) / sizeof(ithoDemandFlowSettingsMap[0]), ithoDemandFlowSettingsLabels, ithoDemandFlowStatusMap, sizeof(ithoDemandFlowStatusMap) / sizeof(ithoDemandFlowStatusMap[0]), ithoDemandFlowStatusLabels},
    {0x00, 0x0C, "CO2 relay", nullptr, 0, nullptr, nullptr, 0, nullptr},
    {0x00, 0x0D, "Heatpump", ithoWPUSettingsMap, sizeof(ithoWPUSettingsMap) / sizeof(ithoWPUSettingsMap[0]), ithoWPUSettingsLabels, ithoWPUStatusMap, sizeof(ithoWPUStatusMap) / sizeof(ithoWPUStatusMap[0]), ithoWPUStatusLabels},
    {0x00, 0x0E, "OLB Single", nullptr, 0, nullptr, nullptr, 0, nullptr},
    {0x00, 0x0F, "AutoTemp", ithoAutoTempSettingsMap, sizeof(ithoAutoTempSettingsMap) / sizeof(ithoAutoTempSettingsMap[0]), ithoAutoTempSettingsLabels, ithoAutoTempStatusMap, sizeof(ithoAutoTempStatusMap) / sizeof(ithoAutoTempStatusMap[0]), ithoAutoTempStatusLabels},
    {0x00, 0x10, "OLB Double", nullptr, 0, nullptr, nullptr, 0, nullptr},
    {0x00, 0x11, "RF+", nullptr, 0, nullptr, nullptr, 0, nullptr},
    {0x00, 0x14, "CVE", itho14SettingsMap, sizeof(itho14SettingsMap) / sizeof(itho14SettingsMap[0]), ithoCVE14SettingsLabels, itho14StatusMap, sizeof(itho14StatusMap) / sizeof(itho14StatusMap[0]), ithoCVE14StatusLabels},
    {0x00, 0x15, "Extended", nullptr, 0, nullptr, nullptr, 0, nullptr},
    {0x00, 0x16, "Extended Plus", nullptr, 0, nullptr, nullptr, 0, nullptr},
    {0x00, 0x1A, "AreaFlow", nullptr, 0, nullptr, nullptr, 0, nullptr},
    {0x00, 0x1B, "CVE-Silent", itho1BSettingsMap, sizeof(itho1BSettingsMap) / sizeof(itho1BSettingsMap[0]), ithoCVE1BSettingsLabels, itho1BStatusMap, sizeof(itho1BStatusMap) / sizeof(itho1BStatusMap[0]), ithoCVE1BStatusLabels},
    {0x00, 0x1C, "CVE-SilentExt", nullptr, 0, nullptr, nullptr, 0, nullptr},
    {0x00, 0x1D, "CVE-SilentExtPlus", ithoHRU200SettingsMap, sizeof(ithoHRU200SettingsMap) / sizeof(ithoHRU200SettingsMap[0]), ithoHRU200SettingsLabels, ithoHRU200StatusMap, sizeof(ithoHRU200StatusMap) / sizeof(ithoHRU200StatusMap[0]), ithoHRU200StatusLabels},
    {0x00, 0x20, "RF_CO2", nullptr, 0, nullptr, nullptr, 0, nullptr},
    {0x00, 0x2B, "HRU 350", ithoHRU350SettingsMap, sizeof(ithoHRU350SettingsMap) / sizeof(ithoHRU350SettingsMap[0]), ithoHRU350SettingsLabels, ithoHRU350StatusMap, sizeof(ithoHRU350StatusMap) / sizeof(ithoHRU350StatusMap[0]), ithoHRU350StatusLabels},
    {0x00, 0x30, "AutoTemp Basic", ithoAutoTempSettingsMap, sizeof(ithoAutoTempSettingsMap) / sizeof(ithoAutoTempSettingsMap[0]), ithoAutoTempSettingsLabels, ithoAutoTempStatusMap, sizeof(ithoAutoTempStatusMap) / sizeof(ithoAutoTempStatusMap[0]), ithoAutoTempStatusLabels},
    {0x07, 0x01, "HRU 250-300", ithoHRU250_300SettingsMap, sizeof(ithoHRU250_300SettingsMap) / sizeof(ithoHRU250_300SettingsMap[0]), ithoHRU250_300SettingsLabels, ithoHRU250_300StatusMap, sizeof(ithoHRU250_300StatusMap) / sizeof(ithoHRU250_300StatusMap[0]), ithoHRU250_300StatusLabels}};

const char *getIthoType()
{
  static char ithoDeviceType[32] = "Unkown device type";

  const struct ihtoDeviceType *ithoDevicesptr = ithoDevices;
  const struct ihtoDeviceType *ithoDevicesendPtr = ithoDevices + sizeof(ithoDevices) / sizeof(ithoDevices[0]);
  while (ithoDevicesptr < ithoDevicesendPtr)
  {
    if (ithoDevicesptr->DG == ithoDeviceGroup && ithoDevicesptr->ID == currentIthoDeviceID())
    {
      strlcpy(ithoDeviceType, ithoDevicesptr->name, sizeof(ithoDeviceType));
    }
    ithoDevicesptr++;
  }
  return ithoDeviceType;
}

int getSettingsLength(const uint8_t deviceGroup, const uint8_t deviceID, const uint8_t version)
{

  const struct ihtoDeviceType *ithoDevicesptr = ithoDevices;
  const struct ihtoDeviceType *ithoDevicesendPtr = ithoDevices + sizeof(ithoDevices) / sizeof(ithoDevices[0]);
  while (ithoDevicesptr < ithoDevicesendPtr)
  {
    if (ithoDevicesptr->DG == deviceGroup && ithoDevicesptr->ID == deviceID)
    {
      if (ithoDevicesptr->settingsMapping == nullptr)
      {
        return -2; // Settings not available for this device
      }
      if (version > (ithoDevicesptr->versionsMapLen - 1))
      {
        return -3; // Version newer than latest version available in the firmware
      }
      if (*(ithoDevicesptr->settingsMapping + version) == nullptr)
      {
        return -4; // Settings not available for this version
      }

      for (int i = 0; i < 999; i++)
      {
        if (static_cast<int>(*(*(ithoDevicesptr->settingsMapping + version) + i) == 999))
        {
          // end of array
          if (ithoSettingsArray == nullptr)
            ithoSettingsArray = new ithoSettings[i];
          return i;
        }
      }
    }
    ithoDevicesptr++;
  }
  return -1;
}

const char *getSettingLabel(const uint8_t index)
{

  const uint8_t deviceID = currentIthoDeviceID();
  const uint8_t version = currentItho_fwversion();
  const uint8_t deviceGroup = currentIthoDeviceGroup();

  int settingsLen = getSettingsLength(deviceGroup, deviceID, version);

  const struct ihtoDeviceType *settingsPtr = ithoDeviceptr;

  if (settingsPtr == nullptr)
  {
    if (systemConfig.api_normalize == 0)
    {
      return ithoLabelErrors[0].labelFull;
    }
    else
    {
      return ithoLabelErrors[0].labelNormalized;
    }
  }
  else if (settingsLen == -2) // Settings not available for this device
  {
    if (systemConfig.api_normalize == 0)
    {
      return ithoLabelErrors[1].labelFull;
    }
    else
    {
      return ithoLabelErrors[1].labelNormalized;
    }
  }
  else if (settingsLen == -3) // Version newer than latest version available in the firmware
  {
    if (systemConfig.api_normalize == 0)
    {
      return ithoLabelErrors[2].labelFull;
    }
    else
    {
      return ithoLabelErrors[2].labelNormalized;
    }
  }
  else if (settingsLen == -4) // Settings not available for this version
  {
    if (systemConfig.api_normalize == 0)
    {
      return ithoLabelErrors[3].labelFull;
    }
    else
    {
      return ithoLabelErrors[3].labelNormalized;
    }
  }
  else if (!(index < settingsLen)) // out of bound
  {
    if (systemConfig.api_normalize == 0)
    {
      return ithoLabelErrors[4].labelFull;
    }
    else
    {
      return ithoLabelErrors[4].labelNormalized;
    }
  }

  return settingsPtr->settingsDescriptions[static_cast<int>(*(*(settingsPtr->settingsMapping + version) + index))];
}

void getSetting(const uint8_t index, const bool updateState, const bool updateweb, const bool loop)
{

  const uint8_t deviceID = currentIthoDeviceID();
  const uint8_t version = currentItho_fwversion();
  const uint8_t deviceGroup = currentIthoDeviceGroup();

  int settingsLen = getSettingsLength(deviceGroup, deviceID, version);
  if (settingsLen < 0)
  {
    logMessagejson("Settings not available for this device or its firmware version", WEBINTERFACE);
    return;
  }

  // const struct ihtoDeviceType *settingsPtr = ithoDeviceptr;

  JsonDocument doc;
  JsonObject root = doc.to<JsonObject>();

  root["Index"] = index;
  root["Description"] = getSettingLabel(index);

  if (!updateState && !updateweb) // -> first run, just send labels and null values
  {
    root["update"] = false;
    root["loop"] = loop;
    root["Current"] = nullptr;
    root["Minimum"] = nullptr;
    root["Maximum"] = nullptr;
    logMessagejson(root, ITHOSETTINGS);
  }
  else if (updateState && !updateweb) // -> 2nd run update setting values of settings page
  {
    // index2410 = index;
    // i2c_result_updateweb = false;
    resultPtr2410 = nullptr;
    // get2410 = true;
    i2c_queue_add_cmd([index]()
                      { resultPtr2410 = sendQuery2410(index, false); });
    i2c_queue_add_cmd([index, loop]()
                      { processSettingResult(index, loop); });
  }
  else // -> update setting values from other source (ie. debug page)
  {
    // index2410 = i;
    // i2c_result_updateweb = updateweb;
    resultPtr2410 = nullptr;
    // get2410 = true;
    i2c_queue_add_cmd([index, updateweb]()
                      { resultPtr2410 = sendQuery2410(index, updateweb); });
  }
}

void processSettingResult(const uint8_t index, const bool loop)
{

  const uint8_t version = currentItho_fwversion();

  const struct ihtoDeviceType *settingsPtr = ithoDeviceptr;

  JsonDocument doc;
  JsonObject root = doc.to<JsonObject>();

  root["Index"] = index;
  root["Description"] = settingsPtr->settingsDescriptions[static_cast<int>(*(*(settingsPtr->settingsMapping + version) + index))];

  auto timeoutmillis = millis() + 3000; // 1 sec. + 2 sec. for potential i2c queue pause on CVE devices
  while (resultPtr2410 == nullptr && millis() < timeoutmillis)
  {
    // wait for result
  }
  root["update"] = true;
  root["loop"] = loop;
  if (resultPtr2410 != nullptr && ithoSettingsArray != nullptr)
  {
    double cur = 0.0; // use doubles instead of float here because ArduinoJson uses double precision internally which would otherwise result in rounding issues. issue #241
    double min = 0.0;
    double max = 0.0;

    if (decodeQuery2410(resultPtr2410, &ithoSettingsArray[index], &cur, &min, &max))
    {
      root["Current"] = cur;
      root["Minimum"] = min;
      root["Maximum"] = max;
    }
    else
    {
      root["Current"] = "ret_error";
      root["Minimum"] = "ret_error";
      root["Maximum"] = "ret_error";
    }
  }
  else
  {
    root["Current"] = nullptr;
    root["Minimum"] = nullptr;
    root["Maximum"] = nullptr;
  }
  logMessagejson(root, ITHOSETTINGS);
}

int getStatusLabelLength(const uint8_t deviceGroup, const uint8_t deviceID, const uint8_t version)
{

  const struct ihtoDeviceType *ithoDevicesptr = ithoDevices;
  const struct ihtoDeviceType *ithoDevicesendPtr = ithoDevices + sizeof(ithoDevices) / sizeof(ithoDevices[0]);
  while (ithoDevicesptr < ithoDevicesendPtr)
  {
    if (ithoDevicesptr->DG == deviceGroup && ithoDevicesptr->ID == deviceID)
    {
      if (ithoDevicesptr->statusLabelMapping == nullptr)
      {
        return -2; // Labels not available for this device
      }
      if (version > (ithoDevicesptr->statusMapLen - 1))
      {
        return -3; // Version newer than latest version available in the firmware
      }
      if (*(ithoDevicesptr->statusLabelMapping + version) == nullptr)
      {
        return -4; // Labels not available for this version
      }

      for (int i = 0; i < 255; i++)
      {
        if (static_cast<int>(*(*(ithoDevicesptr->statusLabelMapping + version) + i) == 255))
        {
          // end of array
          return i;
        }
      }
    }
    ithoDevicesptr++;
  }
  return -1;
}

const char *getStatusLabel(const uint8_t i, const struct ihtoDeviceType *statusPtr)
{
  const uint8_t deviceGroup = currentIthoDeviceGroup();
  const uint8_t deviceID = currentIthoDeviceID();
  const uint8_t version = currentItho_fwversion();

  int statusLabelLen = getStatusLabelLength(deviceGroup, deviceID, version);

  if (statusPtr == nullptr)
  {
    if (systemConfig.api_normalize == 0)
    {
      return ithoLabelErrors[0].labelFull;
    }
    else
    {
      return ithoLabelErrors[0].labelNormalized;
    }
  }
  else if (statusLabelLen == -2)
  {
    if (systemConfig.api_normalize == 0)
    {
      return ithoLabelErrors[1].labelFull;
    }
    else
    {
      return ithoLabelErrors[1].labelNormalized;
    }
  }
  else if (statusLabelLen == -3)
  {
    if (systemConfig.api_normalize == 0)
    {
      return ithoLabelErrors[2].labelFull;
    }
    else
    {
      return ithoLabelErrors[2].labelNormalized;
    }
  }
  else if (statusLabelLen == -4)
  {
    if (systemConfig.api_normalize == 0)
    {
      return ithoLabelErrors[3].labelFull;
    }
    else
    {
      return ithoLabelErrors[3].labelNormalized;
    }
  }
  else if (!(i < statusLabelLen))
  {
    if (systemConfig.api_normalize == 0)
    {
      return ithoLabelErrors[4].labelFull;
    }
    else
    {
      return ithoLabelErrors[4].labelNormalized;
    }
  }
  else
  {
    if (systemConfig.api_normalize == 0)
    {
      return (statusPtr->settingsStatusLabels[static_cast<int>(*(*(statusPtr->statusLabelMapping + version) + i))].labelFull);
    }
    else
    {
      return (statusPtr->settingsStatusLabels[static_cast<int>(*(*(statusPtr->statusLabelMapping + version) + i))].labelNormalized);
    }
  }
}

void updateSetting(const uint8_t index, const int32_t value, bool webupdate)
{
  // i2c_result_updateweb = webupdate;
  // index2410 = index;
  // value2410 = value;
  // set2410 = true;

  i2c_queue_add_cmd([index, value, webupdate]()
                    { setSetting2410(index, value, webupdate); });

  i2c_queue_add_cmd([index]()
                    { getSetting(index, true, false, false); });
}

const struct ihtoDeviceType *getDevicePtr(const uint8_t deviceGroup, const uint8_t deviceID)
{

  const struct ihtoDeviceType *ithoDevicesptr = ithoDevices;
  const struct ihtoDeviceType *ithoDevicesendPtr = ithoDevices + sizeof(ithoDevices) / sizeof(ithoDevices[0]);
  while (ithoDevicesptr < ithoDevicesendPtr)
  {
    if (ithoDevicesptr->DG == deviceGroup && ithoDevicesptr->ID == deviceID)
    {
      return ithoDevicesptr;
    }
    ithoDevicesptr++;
  }
  return nullptr;
}

uint8_t checksum(const uint8_t *buf, size_t buflen)
{
  uint8_t sum = 0;
  while (buflen--)
  {
    sum += *buf++;
  }
  return -sum;
}

void sendI2CPWMinit()
{

  // uint8_t command[] = {0x82, 0xEF, 0xC0, 0x00, 0x01, 0x06, 0x00, 0x00, 0x09, 0x00, 0x09, 0x00, 0xB6};
  // 82 EF CO 00 01 06 00 00 13 00 13 00 A2
  uint8_t command[] = {0x82, 0xEF, 0xC0, 0x00, 0x01, 0x06, 0x00, 0x00, 0x13, 0x00, 0x13, 0x00, 0xB6};

  command[sizeof(command) - 1] = checksum(command, sizeof(command) - 1);

  i2c_sendBytes(command, sizeof(command), I2C_CMD_PWM_INIT);
}

// init 82 EF CO 00 01 06 00 00 13 00 13 00 A2
void sendCO2init()
{
  uint8_t command[] = {0x82, 0xEF, 0xC0, 0x00, 0x01, 0x06, 0x00, 0x00, 0x13, 0x00, 0x13, 0x00, 0xA2};
  command[sizeof(command) - 1] = checksum(command, sizeof(command) - 1);

  i2c_sendBytes(command, sizeof(command), I2C_CMD_CO2_INIT);
}

void sendCO2query(bool updateweb)
{
  uint8_t command[] = {0x82, 0xEF, 0xC0, 0x00, 0x01, 0x06, 0x00, 0x00, 0x13, 0x00, 0x13, 0x00, 0xA2};
  uint8_t i2cbuf[512]{};

  size_t result = send_i2c_query(command, sizeof(command), i2cbuf, I2C_CMD_QUERY_DEVICE_TYPE);
  if (!result)
  {
    if (updateweb)
    {
      updateweb = false;
      jsonSysmessage("sendCO2init", "failed");
    }

    return;
  }
  else
  {
    if (updateweb)
    {
      updateweb = false;
      jsonSysmessage("sendCO2init", i2cbuf2string(i2cbuf, result).c_str());
    }
  }
}

// 60 B1 E0 01 03 00 66 02 A3

void sendCO2speed(uint8_t speed1, uint8_t speed2)
{
  //                    0     1     2     3     4     5     6     7     8     9     10    11    12    13    14    15    16    17    18   19    20    21
  uint8_t command[] = {0x82, 0xC1, 0x00, 0x01, 0x10, 0x14, 0x51, 0x13, 0x2D, 0x31, 0xE0, 0x08, 0x00, 0x00, 0xFF, 0x01, 0x01, 0x00, 0xFF, 0x01, 0xFF, 0xAC};
  //                   0x82, 0xC1, 0x00, 0x01, 0x10, 0x14, 0x51, 0x13, 0x2D, 0x31, 0xE0, 0x08, 0x00, 0x00, 0x12, 0x01, 0x01, 0x00, 0x0A, 0x01, 0x23, 0xAC
  command[14] = speed1;
  command[18] = speed2;

  // calculate chk2 val
  uint8_t i2c_command_tmp[32] = {0};
  // command[11] == 9;

  uint8_t i2c_command_startpos = 5;
  uint8_t i2c_command_endpos = sizeof(command) - 2;

  for (int i = i2c_command_startpos; i < i2c_command_endpos; i++)
  {
    i2c_command_tmp[(i - i2c_command_startpos)] = command[i];
  }
  command[sizeof(command) - 2] = checksum(i2c_command_tmp, i2c_command_endpos - i2c_command_startpos); // FIXME: check set correct byte, last or second last?

  for (int i = 0; i < sizeof(command); i++)
  {
    D_LOG("0x%02X, ", command[i]);
  }

  i2c_sendBytes(command, sizeof(command), I2C_CMD_CO2_CMD);
}
void sendCO2value(uint16_t value)
{
  //                    0     1     2     3     4     5     6     7
  uint8_t command[] = {0x60, 0x92, 0x98, 0x01, 0x02, 0xFF, 0xFF, 0xFF};
  //                  {0x60, 0x92, 0x98, 0x01, 0x02, 0x03, 0x25, 0x4B}; 805ppm
  command[5] = value >> 8;
  command[6] = value & 0xFF;

  command[sizeof(command) - 1] = checksum(command, sizeof(command) - 1);

  D_LOG("sendCO2value:%d", value);
  for (int i = 0; i < sizeof(command); i++)
  {
    D_LOG("0x%02X, ", command[i]);
  }

  i2c_sendBytes(command, sizeof(command), I2C_CMD_CO2_CMD);
}

uint8_t cmdCounter = 0;

void sendRemoteCmd(const uint8_t remoteIndex, const IthoCommand command)
{
  // command structure:
  //  [I2C addr ][  I2C command   ][len ][    timestamp         ][fmt ][    remote ID   ][cntr]<  opcode  ><len2><   len2 length command      >[chk2][cntr][chk ]
  //  0x82, 0x60, 0xC1, 0x01, 0x01, 0x09, 0xFF, 0xFF, 0xFF, 0xFF, 0x16, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x05, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
  if (remoteIndex > virtualRemotes.getMaxRemotes())
    return;

  const RemoteTypes remoteType = virtualRemotes.getRemoteType(remoteIndex);
  if (remoteType == RemoteTypes::UNSETTYPE)
    return;

  // Get the corresponding command / remote type combination
  const uint8_t *remote_command = rf.getRemoteCmd(remoteType, command);
  if (remote_command == nullptr)
    return;

  uint8_t i2c_command[64] = {0};
  uint8_t i2c_command_len = 0;

  /*
     First build the i2c header / wrapper for the remote command

  */
  //                       [I2C addr ][  I2C command   ][len ][    timestamp         ][fmt ][    remote ID   ][cntr]
  uint8_t i2c_header[] = {0x82, 0x60, 0xC1, 0x01, 0x01, 0x09, 0xFF, 0xFF, 0xFF, 0xFF, 0x16, 0xFF, 0xFF, 0xFF, 0xFF};

  unsigned long curtime = millis();
  i2c_header[6] = (curtime >> 24) & 0xFF;
  i2c_header[7] = (curtime >> 16) & 0xFF;
  i2c_header[8] = (curtime >> 8) & 0xFF;
  i2c_header[9] = curtime & 0xFF;

  uint8_t id[3]{};
  virtualRemotes.getRemoteIDbyIndex(remoteIndex, &id[0]);
  i2c_header[11] = id[0];
  i2c_header[12] = id[1];
  i2c_header[13] = id[2];

  i2c_header[14] = cmdCounter;
  cmdCounter++;

  for (; i2c_command_len < sizeof(i2c_header) / sizeof(i2c_header[0]); i2c_command_len++)
  {
    i2c_command[i2c_command_len] = i2c_header[i2c_command_len];
  }

  // determine command length
  const int command_len = remote_command[2];

  // copy to i2c_command
  for (int i = 0; i < 2 + command_len + 1; i++)
  {
    i2c_command[i2c_command_len] = remote_command[i];

    if (i > 5 && (command == IthoJoin || command == IthoLeave))
    {
      // command bytes locations with ID in Join/Leave messages: 6/7/8 12/13/14 18/19/20 24/25/26 30/31/32 36/37/38 etc
      if (i % 6 == 0 || i % 6 == 1 || i % 6 == 2)
      {
        i2c_command[i2c_command_len] = *(id + (i % 6));
      }
    }
    i2c_command_len++;
  }

  // // if join or leave, add remote ID fields
  // if (command == IthoJoin || command == IthoLeave)
  // {
  //   // if len = 6/7/8 12/13/14 18/19/20 24/25/26 30/31/32 36/37/38 etc... continue; those are the device ID bytes on Join/Leave messages
  //   if (> 5 && (i % 6 == 0 || i % 6 == 1 || i % 6 == 2))
  //     continue;
  //   // set command ID's
  //   if (command_len > 0x05)
  //   {
  //     // add 1st ID
  //     i2c_command[21] = *id;
  //     i2c_command[22] = *(id + 1);
  //     i2c_command[23] = *(id + 2);
  //   }
  //   if (command_len > 0x0B)
  //   {
  //     // add 2nd ID
  //     i2c_command[27] = *id;
  //     i2c_command[28] = *(id + 1);
  //     i2c_command[29] = *(id + 2);
  //   }
  //   if (command_len > 0x12)
  //   {
  //     // add 3rd ID
  //     i2c_command[33] = *id;
  //     i2c_command[34] = *(id + 1);
  //     i2c_command[35] = *(id + 2);
  //   }
  //   if (command_len > 0x17)
  //   {
  //     // add 4th ID
  //     i2c_command[39] = *id;
  //     i2c_command[40] = *(id + 1);
  //     i2c_command[41] = *(id + 2);
  //   }
  //   if (command_len > 0x1D)
  //   {
  //     // add 5th ID
  //     i2c_command[45] = *id;
  //     i2c_command[46] = *(id + 1);
  //     i2c_command[47] = *(id + 2);
  //   }
  // }

  // if timer1-3 command use timer config of systemConfig.itho_timer1-3
  // example timer command: {0x22, 0xF3, 0x06, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00} 0xFF is timer value on pos 20 of i2c_command[]
  if (command == IthoTimer1)
  {
    i2c_command[20] = systemConfig.itho_timer1;
  }
  else if (command == IthoTimer2)
  {
    i2c_command[20] = systemConfig.itho_timer2;
  }
  else if (command == IthoTimer3)
  {
    i2c_command[20] = systemConfig.itho_timer3;
  }

  /*
     built the footer of the i2c wrapper
     chk2 = checksum of [fmt]+[remote ID]+[cntr]+[remote command]
     chk = checksum of the whole command

  */
  //                        [chk2][cntr][chk ]
  // uint8_t i2c_footer[] = { 0x00, 0x00, 0xFF };

  // calculate chk2 val
  uint8_t i2c_command_tmp[64] = {0};
  for (int i = 10; i < i2c_command_len; i++)
  {
    i2c_command_tmp[(i - 10)] = i2c_command[i];
  }

  i2c_command[i2c_command_len] = checksum(i2c_command_tmp, i2c_command_len - 11);
  i2c_command_len++;

  //[cntr]
  i2c_command[i2c_command_len] = 0x00;
  i2c_command_len++;

  // set i2c_command length value
  i2c_command[5] = i2c_command_len - 6;

  // calculate chk val
  i2c_command[i2c_command_len] = checksum(i2c_command, i2c_command_len - 1);
  i2c_command_len++;

#if defined(ENABLE_SERIAL)
  String str;
  char buf[250] = {0};
  for (int i = 0; i < i2c_command_len; i++)
  {
    snprintf(buf, sizeof(buf), " 0x%02X", i2c_command[i]);
    str += String(buf);
    if (i < i2c_command_len - 1)
    {
      str += ",";
    }
  }
  str += "\n";
  D_LOG(str.c_str());
#endif

  i2c_sendBytes(i2c_command, i2c_command_len, I2C_CMD_REMOTE_CMD);
}

void sendQueryDevicetype(bool updateweb)
{
  uint8_t command[] = {0x82, 0x80, 0x90, 0xE0, 0x04, 0x00, 0x8A};
  uint8_t i2cbuf[512]{};

  size_t result = send_i2c_query(command, sizeof(command), i2cbuf, I2C_CMD_QUERY_DEVICE_TYPE);
  // example send_i2c_query reply: 80 82 90 E0 01 25 00 01 00 1B 39 1B 01 FE FF FF FF FF FF 04 0B 07 E5 43 56 45 2D 52 46 00 00 00 00 00 00 00 00 00 00 00 00 00 00 60

  if (!result)
  {
    if (updateweb)
    {
      updateweb = false;
      jsonSysmessage("ithotype", "failed");
    }

    return;
  }
  else
  {
    if (updateweb)
    {
      updateweb = false;
      jsonSysmessage("ithotype", i2cbuf2string(i2cbuf, result).c_str());
    }
  }

  ithoDeviceGroup = i2cbuf[8];
  ithoDeviceID = i2cbuf[9];
  itho_hwversion = i2cbuf[10];
  itho_fwversion = i2cbuf[11];
  ithoDeviceptr = getDevicePtr(currentIthoDeviceGroup(), currentIthoDeviceID());
  ithoSettingsLength = getSettingsLength(currentIthoDeviceGroup(), currentIthoDeviceID(), currentItho_fwversion());
  restest = true;
}

void sendQueryStatusFormat(bool updateweb)
{

  uint8_t command[] = {0x82, 0x80, 0x24, 0x00, 0x04, 0x00, 0xD6};
  uint8_t i2cbuf[512]{};

  size_t result = send_i2c_query(&command[0], sizeof(command), &i2cbuf[0], I2C_CMD_QUERY_STATUS_FORMAT);
  // example send_i2c_query reply: 80 82 A4 00 01 0C 80 10 10 10 00 10 20 10 10 00 92 92 29
  if (!result)
  {
    if (updateweb)
    {
      updateweb = false;
      jsonSysmessage("ithostatusformat", "failed");
    }
    return;
  }
  else
  {
    if (updateweb)
    {
      updateweb = false;
      jsonSysmessage("ithostatusformat", i2cbuf2string(i2cbuf, result).c_str());
    }
  }

  if (!ithoStatus.empty())
  {
    ithoStatus.clear();
  }
  if (!(currentItho_fwversion() > 0))
    return;
  ithoStatusLabelLength = getStatusLabelLength(currentIthoDeviceGroup(), currentIthoDeviceID(), currentItho_fwversion());
  const uint8_t endPos = i2cbuf[5];

  for (uint8_t i = 0; i < endPos; i++)
  {
    ithoStatus.push_back(ithoDeviceStatus());

    //      char fStringBuf[32];
    //      getStatusLabel(i, ithoDeviceptr, currentItho_fwversion(), fStringBuf);

    ithoStatus.back().is_signed = get_signed_from_datatype(i2cbuf[6 + i]);
    ithoStatus.back().length = get_length_from_datatype(i2cbuf[6 + i]);
    ithoStatus.back().divider = get_divider_from_datatype(i2cbuf[6 + i]);

    if (ithoStatus.back().divider == 1)
    { // integer value
      ithoStatus.back().type = ithoDeviceStatus::is_int;
    }
    else
    {
      ithoStatus.back().type = ithoDeviceStatus::is_float;
    }
    // special cases
    if (i2cbuf[6 + i] == 0x5B)
    {
      // legacy itho: 0x5B -> 0x10
      ithoStatus.back().type = ithoDeviceStatus::is_int;
      ithoStatus.back().length = 2;
      ithoStatus.back().is_signed = false;
    }
  }
}

void sendQueryStatus(bool updateweb)
{

  uint8_t command[] = {0x82, 0x80, 0x24, 0x01, 0x04, 0x00, 0xD5};
  uint8_t i2cbuf[512]{};

  size_t result = send_i2c_query(&command[0], sizeof(command), &i2cbuf[0], I2C_CMD_QUERY_STATUS);
  // example send_i2c_query reply: 80 82 A4 01 01 17 02 03 2C 03 2E 00 80 07 01 2A 00 00 13 2A 00 00 82 00 2C 11 6C 09 16 A6

  if (!result)
  {
    if (updateweb)
    {
      updateweb = false;
      jsonSysmessage("ithostatus", "failed");
    }
    return;
  }
  else
  {
    if (updateweb)
    {
      updateweb = false;
      jsonSysmessage("ithostatus", i2cbuf2string(i2cbuf, result).c_str());
    }
  }

  int statusPos = 6; // first byte with status info
  int labelPos = 0;
  if (!ithoStatus.empty())
  {
    for (auto &ithoStat : ithoStatus)
    {
      ithoStat.name = getStatusLabel(labelPos, ithoDeviceptr);
      auto tempVal = 0;
      for (int i = ithoStat.length; i > 0; i--)
      {
        tempVal |= i2cbuf[statusPos + (ithoStat.length - i)] << ((i - 1) * 8);
      }

      if (ithoStat.type == ithoDeviceStatus::is_byte)
      {
        if (ithoStat.value.byteval == (byte)tempVal)
        {
          ithoStat.updated = 0;
        }
        else
        {
          ithoStat.updated = 1;
          ithoStat.value.byteval = (byte)tempVal;
        }
      }
      if (ithoStat.type == ithoDeviceStatus::is_int)
      {
        if (ithoStat.is_signed)
        {
          // interpret raw bytes as signed integer
          tempVal = cast_to_signed_int(tempVal, ithoStat.length);
        }
        if (ithoStat.value.intval == tempVal)
        {
          ithoStat.updated = 0;
        }
        else
        {
          ithoStat.updated = 1;
          ithoStat.value.intval = tempVal;
        }
      }
      if (ithoStat.type == ithoDeviceStatus::is_float)
      {
        float t = ithoStat.value.floatval * ithoStat.divider;
        if (static_cast<uint32_t>(t) == tempVal) // better compare needed of float val, worst case this will result in an extra update of the value, so limited impact
        {
          ithoStat.updated = 0;
        }
        else
        {
          ithoStat.updated = 1;
          if (ithoStat.is_signed)
          {
            // interpret raw bytes as signed integer
            tempVal = cast_to_signed_int(tempVal, ithoStat.length);
          }
          ithoStat.value.floatval = static_cast<float>(tempVal) / ithoStat.divider;
        }
      }

      statusPos += ithoStat.length;

      if (strcmp("Highest CO2 concentration (ppm)", ithoStat.name) == 0 || strcmp("highest-co2-concentration_ppm", ithoStat.name) == 0)
      {
        if (ithoStat.value.intval == 0x8200)
        {
          ithoStat.type = ithoDeviceStatus::is_string;
          ithoStat.value.stringval = fanSensorErrors.begin()->second;
        }
      }
      if (strcmp("Highest RH concentration (%)", ithoStat.name) == 0 || strcmp("highest-rh-concentration_perc", ithoStat.name) == 0)
      {
        if (ithoStat.value.intval == 0xEF)
        {
          ithoStat.type = ithoDeviceStatus::is_string;
          ithoStat.value.stringval = fanSensorErrors.begin()->second;
        }
      }
      if (strcmp("RelativeHumidity", ithoStat.name) == 0 || strcmp("relativehumidity", ithoStat.name) == 0)
      {
        if (ithoStat.value.floatval > 1.0 && ithoStat.value.floatval < 100.0)
        {
          if (systemConfig.syssht30 == 0)
          {
            ithoHum = ithoStat.value.floatval;
            itho_internal_hum_temp = true;
          }
        }
        else
        {
          ithoStat.type = ithoDeviceStatus::is_string;
          ithoStat.value.stringval = fanSensorErrors.begin()->second;
        }
      }
      if (strcmp("Temperature", ithoStat.name) == 0 || strcmp("temperature", ithoStat.name) == 0)
      {
        if (ithoStat.value.floatval > 1.0 && ithoStat.value.floatval < 100.0)
        {
          if (systemConfig.syssht30 == 0)
          {
            ithoTemp = ithoStat.value.floatval;
            itho_internal_hum_temp = true;
          }
        }
        else
        {
          ithoStat.type = ithoDeviceStatus::is_string;
          ithoStat.value.stringval = fanSensorErrors.begin()->second;
        }
      }

      labelPos++;
    }
  }
}

void sendQuery31DA(bool updateweb)
{
  uint8_t i2cbuf[512]{};
  size_t result = sendQuery31DA(&i2cbuf[0]);
  // example send_i2c_query reply: 80 82 B1 DA 01 1D EF 00 7F FF 2C EF 7F FF 7F FF 7F FF 7F FF F8 08 EF 98 05 00 00 00 EF EF 7F FF 7F FF 00 6F
  if (!result)
  {
    if (updateweb)
    {
      updateweb = false;
      jsonSysmessage("itho31DA", "failed");
    }
    return;
  }
  else
  {
    if (updateweb)
    {
      updateweb = false;
      jsonSysmessage("itho31DA", i2cbuf2string(i2cbuf, result).c_str());
    }
  }

  auto dataLength = i2cbuf[5];

  auto dataStart = 6;
  if (!ithoMeasurements.empty())
  {
    ithoMeasurements.clear();
  }
  const int labelLen = 19;
  static const char *labels31DA[labelLen] = {0};
  for (int i = 0; i < labelLen; i++)
  {
    if (systemConfig.api_normalize == 0)
    {
      labels31DA[i] = itho31DALabels[i].labelFull;
    }
    else
    {
      labels31DA[i] = itho31DALabels[i].labelNormalized;
    }
  }
  if (dataLength > 0)
  {
    ithoMeasurements.push_back(ithoDeviceMeasurements());
    ithoMeasurements.back().name = labels31DA[0];
    if (i2cbuf[0 + dataStart] > 200)
    {
      ithoMeasurements.back().type = ithoDeviceMeasurements::is_string;
      auto it = fanSensorErrors.find(i2cbuf[0 + dataStart]);
      if (it != fanSensorErrors.end())
        ithoMeasurements.back().value.stringval = it->second;
      else
        ithoMeasurements.back().value.stringval = fanSensorErrors.rbegin()->second;
    }
    else
    {
      ithoMeasurements.back().type = ithoDeviceMeasurements::is_int;
      ithoMeasurements.back().value.intval = i2cbuf[0 + dataStart];
    }

    ithoMeasurements.push_back(ithoDeviceMeasurements());
    ithoMeasurements.back().name = labels31DA[1];
    ithoMeasurements.back().type = ithoDeviceMeasurements::is_int;
    ithoMeasurements.back().value.intval = i2cbuf[1 + dataStart];
  }
  if (dataLength > 1)
  {
    ithoMeasurements.push_back(ithoDeviceMeasurements());
    ithoMeasurements.back().name = labels31DA[2];
    if (i2cbuf[2 + dataStart] >= 0x7F)
    {
      ithoMeasurements.back().type = ithoDeviceMeasurements::is_string;
      auto it = fanSensorErrors2.find(i2cbuf[2 + dataStart]);
      if (it != fanSensorErrors2.end())
        ithoMeasurements.back().value.stringval = it->second;
      else
        ithoMeasurements.back().value.stringval = fanSensorErrors2.rbegin()->second;
    }
    else
    {
      int32_t tempVal = i2cbuf[2 + dataStart] << 8;
      tempVal |= i2cbuf[3 + dataStart];
      ithoMeasurements.back().type = ithoDeviceMeasurements::is_int;
      ithoMeasurements.back().value.intval = tempVal;
    }
  }
  if (dataLength > 3)
  {
    ithoMeasurements.push_back(ithoDeviceMeasurements());
    ithoMeasurements.back().name = labels31DA[3];
    if (i2cbuf[4 + dataStart] > 200)
    {
      ithoMeasurements.back().type = ithoDeviceMeasurements::is_string;
      auto it = fanSensorErrors.find(i2cbuf[4 + dataStart]);
      if (it != fanSensorErrors.end())
        ithoMeasurements.back().value.stringval = it->second;
      else
        ithoMeasurements.back().value.stringval = fanSensorErrors.rbegin()->second;
    }
    else
    {
      ithoMeasurements.back().type = ithoDeviceMeasurements::is_int;
      ithoMeasurements.back().value.intval = i2cbuf[4 + dataStart];
    }
  }
  if (dataLength > 4)
  {
    ithoMeasurements.push_back(ithoDeviceMeasurements());
    ithoMeasurements.back().name = labels31DA[4];
    if (i2cbuf[5 + dataStart] > 200)
    {
      ithoMeasurements.back().type = ithoDeviceMeasurements::is_string;
      auto it = fanSensorErrors.find(i2cbuf[5 + dataStart]);
      if (it != fanSensorErrors.end())
        ithoMeasurements.back().value.stringval = it->second;
      else
        ithoMeasurements.back().value.stringval = fanSensorErrors.rbegin()->second;
    }
    else
    {
      ithoMeasurements.back().type = ithoDeviceMeasurements::is_float;
      ithoMeasurements.back().value.floatval = i2cbuf[5 + dataStart] / 2;
    }
  }
  if (dataLength > 5)
  {
    ithoMeasurements.push_back(ithoDeviceMeasurements());
    ithoMeasurements.back().name = labels31DA[5];
    if (i2cbuf[6 + dataStart] >= 0x7F)
    {
      ithoMeasurements.back().type = ithoDeviceMeasurements::is_string;
      auto it = fanSensorErrors2.find(i2cbuf[6 + dataStart]);
      if (it != fanSensorErrors2.end())
        ithoMeasurements.back().value.stringval = it->second;
      else
        ithoMeasurements.back().value.stringval = fanSensorErrors2.rbegin()->second;
    }
    else
    {
      int32_t tempVal = i2cbuf[6 + dataStart] << 8;
      tempVal |= i2cbuf[7 + dataStart];
      ithoMeasurements.back().type = ithoDeviceMeasurements::is_float;
      ithoMeasurements.back().value.floatval = tempVal / 100.0;
    }
  }
  if (dataLength > 7)
  {
    ithoMeasurements.push_back(ithoDeviceMeasurements());
    ithoMeasurements.back().name = labels31DA[6];
    if (i2cbuf[8 + dataStart] >= 0x7F)
    {
      ithoMeasurements.back().type = ithoDeviceMeasurements::is_string;
      auto it = fanSensorErrors2.find(i2cbuf[8 + dataStart]);
      if (it != fanSensorErrors2.end())
        ithoMeasurements.back().value.stringval = it->second;
      else
        ithoMeasurements.back().value.stringval = fanSensorErrors2.rbegin()->second;
    }
    else
    {
      int32_t tempVal = i2cbuf[8 + dataStart] << 8;
      tempVal |= i2cbuf[9 + dataStart];
      ithoMeasurements.back().type = ithoDeviceMeasurements::is_float;
      ithoMeasurements.back().value.floatval = tempVal / 100.0;
    }
  }
  if (dataLength > 9)
  {
    ithoMeasurements.push_back(ithoDeviceMeasurements());
    ithoMeasurements.back().name = labels31DA[7];
    if (i2cbuf[10 + dataStart] >= 0x7F)
    {
      ithoMeasurements.back().type = ithoDeviceMeasurements::is_string;
      auto it = fanSensorErrors2.find(i2cbuf[10 + dataStart]);
      if (it != fanSensorErrors2.end())
        ithoMeasurements.back().value.stringval = it->second;
      else
        ithoMeasurements.back().value.stringval = fanSensorErrors2.rbegin()->second;
    }
    else
    {
      int32_t tempVal = i2cbuf[10 + dataStart] << 8;
      tempVal |= i2cbuf[11 + dataStart];
      ithoMeasurements.back().type = ithoDeviceMeasurements::is_float;
      ithoMeasurements.back().value.floatval = tempVal / 100.0;
    }
  }
  if (dataLength > 11)
  {
    ithoMeasurements.push_back(ithoDeviceMeasurements());
    ithoMeasurements.back().name = labels31DA[8];
    if (i2cbuf[12 + dataStart] >= 0x7F)
    {
      ithoMeasurements.back().type = ithoDeviceMeasurements::is_string;
      auto it = fanSensorErrors2.find(i2cbuf[12 + dataStart]);
      if (it != fanSensorErrors2.end())
        ithoMeasurements.back().value.stringval = it->second;
      else
        ithoMeasurements.back().value.stringval = fanSensorErrors2.rbegin()->second;
    }
    else
    {
      int32_t tempVal = i2cbuf[12 + dataStart] << 8;
      tempVal |= i2cbuf[13 + dataStart];
      ithoMeasurements.back().type = ithoDeviceMeasurements::is_float;
      ithoMeasurements.back().value.floatval = tempVal / 100.0;
    }
  }
  if (dataLength > 13)
  {
    ithoMeasurements.push_back(ithoDeviceMeasurements());
    ithoMeasurements.back().name = labels31DA[9];
    ithoMeasurements.back().type = ithoDeviceMeasurements::is_int;
    int32_t tempVal = i2cbuf[14 + dataStart] << 8;
    tempVal |= i2cbuf[15 + dataStart];
    ithoMeasurements.back().value.intval = tempVal;
  }
  if (dataLength > 15)
  {
    ithoMeasurements.push_back(ithoDeviceMeasurements());
    ithoMeasurements.back().name = labels31DA[10];
    if (i2cbuf[16 + dataStart] > 200)
    {
      ithoMeasurements.back().type = ithoDeviceMeasurements::is_string;
      auto it = fanSensorErrors.find(i2cbuf[16 + dataStart]);
      if (it != fanSensorErrors.end())
        ithoMeasurements.back().value.stringval = it->second;
      else
        ithoMeasurements.back().value.stringval = fanSensorErrors.rbegin()->second;
    }
    else
    {
      ithoMeasurements.back().type = ithoDeviceMeasurements::is_float;
      ithoMeasurements.back().value.floatval = i2cbuf[16 + dataStart] / 2;
    }
  }
  if (dataLength > 16)
  {
    ithoMeasurements.push_back(ithoDeviceMeasurements());
    ithoMeasurements.back().name = labels31DA[11];
    ithoMeasurements.back().type = ithoDeviceMeasurements::is_string;
    auto it = fanInfo.find(i2cbuf[17 + dataStart] & 0x1F);
    if (it != fanInfo.end())
      ithoMeasurements.back().value.stringval = it->second;
    else
      ithoMeasurements.back().value.stringval = fanInfo.rbegin()->second;
  }
  if (dataLength > 17)
  {
    ithoMeasurements.push_back(ithoDeviceMeasurements());
    ithoMeasurements.back().name = labels31DA[12];
    if (i2cbuf[18 + dataStart] > 200)
    {
      ithoMeasurements.back().type = ithoDeviceMeasurements::is_string;
      auto it = fanSensorErrors.find(i2cbuf[18 + dataStart]);
      if (it != fanSensorErrors.end())
        ithoMeasurements.back().value.stringval = it->second;
      else
        ithoMeasurements.back().value.stringval = fanSensorErrors.rbegin()->second;
    }
    else
    {
      ithoMeasurements.back().type = ithoDeviceMeasurements::is_float;
      ithoMeasurements.back().value.floatval = i2cbuf[18 + dataStart] / 2;
    }
  }
  if (dataLength > 18)
  {
    ithoMeasurements.push_back(ithoDeviceMeasurements());
    ithoMeasurements.back().name = labels31DA[13];
    if (i2cbuf[19 + dataStart] > 200)
    {
      ithoMeasurements.back().type = ithoDeviceMeasurements::is_string;
      auto it = fanSensorErrors.find(i2cbuf[19 + dataStart]);
      if (it != fanSensorErrors.end())
        ithoMeasurements.back().value.stringval = it->second;
      else
        ithoMeasurements.back().value.stringval = fanSensorErrors.rbegin()->second;
    }
    else
    {
      ithoMeasurements.back().type = ithoDeviceMeasurements::is_float;
      ithoMeasurements.back().value.floatval = i2cbuf[19 + dataStart] / 2;
    }
  }
  if (dataLength > 19)
  {
    ithoMeasurements.push_back(ithoDeviceMeasurements());
    ithoMeasurements.back().name = labels31DA[14];
    int32_t tempVal = i2cbuf[20 + dataStart] << 8;
    tempVal |= i2cbuf[21 + dataStart];
    ithoMeasurements.back().type = ithoDeviceMeasurements::is_int;
    ithoMeasurements.back().value.intval = tempVal;
  }
  if (dataLength > 21)
  {
    ithoMeasurements.push_back(ithoDeviceMeasurements());
    ithoMeasurements.back().name = labels31DA[15];
    if (i2cbuf[22 + dataStart] > 200)
    {
      ithoMeasurements.back().type = ithoDeviceMeasurements::is_string;
      auto it = fanHeatErrors.find(i2cbuf[22 + dataStart]);
      if (it != fanHeatErrors.end())
        ithoMeasurements.back().value.stringval = it->second;
      else
        ithoMeasurements.back().value.stringval = fanHeatErrors.rbegin()->second;
    }
    else
    {
      ithoMeasurements.back().type = ithoDeviceMeasurements::is_float;
      ithoMeasurements.back().value.floatval = i2cbuf[22 + dataStart] / 2;
    }
  }
  if (dataLength > 22)
  {
    ithoMeasurements.push_back(ithoDeviceMeasurements());
    ithoMeasurements.back().name = labels31DA[16];
    if (i2cbuf[23 + dataStart] > 200)
    {
      ithoMeasurements.back().type = ithoDeviceMeasurements::is_string;
      auto it = fanHeatErrors.find(i2cbuf[23 + dataStart]);
      if (it != fanHeatErrors.end())
        ithoMeasurements.back().value.stringval = it->second;
      else
        ithoMeasurements.back().value.stringval = fanHeatErrors.rbegin()->second;
    }
    else
    {
      ithoMeasurements.back().type = ithoDeviceMeasurements::is_float;
      ithoMeasurements.back().value.floatval = i2cbuf[23 + dataStart] / 2;
    }
  }
  if (dataLength > 23)
  {
    ithoMeasurements.push_back(ithoDeviceMeasurements());
    ithoMeasurements.back().name = labels31DA[17];
    if (i2cbuf[24 + dataStart] >= 0x7F)
    {
      ithoMeasurements.back().type = ithoDeviceMeasurements::is_string;
      auto it = fanSensorErrors2.find(i2cbuf[24 + dataStart]);
      if (it != fanSensorErrors2.end())
        ithoMeasurements.back().value.stringval = it->second;
      else
        ithoMeasurements.back().value.stringval = fanSensorErrors2.rbegin()->second;
    }
    else
    {
      int32_t tempVal = i2cbuf[24 + dataStart] << 8;
      tempVal |= i2cbuf[25 + dataStart];
      ithoMeasurements.back().type = ithoDeviceMeasurements::is_float;
      ithoMeasurements.back().value.floatval = tempVal / 100.0;
    }
  }
  if (dataLength > 25)
  {
    ithoMeasurements.push_back(ithoDeviceMeasurements());
    ithoMeasurements.back().name = labels31DA[18];

    if (i2cbuf[26 + dataStart] >= 0x7F)
    {
      ithoMeasurements.back().type = ithoDeviceMeasurements::is_string;
      auto it = fanSensorErrors2.find(i2cbuf[26 + dataStart]);
      if (it != fanSensorErrors2.end())
        ithoMeasurements.back().value.stringval = it->second;
      else
        ithoMeasurements.back().value.stringval = fanSensorErrors2.rbegin()->second;
    }
    else
    {
      int32_t tempVal = i2cbuf[26 + dataStart] << 8;
      tempVal |= i2cbuf[27 + dataStart];
      ithoMeasurements.back().type = ithoDeviceMeasurements::is_float;
      ithoMeasurements.back().value.floatval = tempVal / 100.0;
    }
  }
}
size_t sendQuery31DA(uint8_t *receive_buffer)
{
  uint8_t command[] = {0x82, 0x80, 0x31, 0xDA, 0x04, 0x00, 0xEF};

  return send_i2c_query(&command[0], sizeof(command), &receive_buffer[0], I2C_CMD_QUERY_31DA);
}

void sendQuery31D9(bool updateweb)
{
  i2c_31d9_done = false;

  uint8_t i2cbuf[512]{};
  size_t result = sendQuery31D9(&i2cbuf[0]);
  // example send_i2c_query reply: 80 82 B1 D9 01 10 86 04 0A 20 20 20 20 20 20 20 20 20 20 20 20 00 4F

  if (!result)
  {
    if (updateweb)
    {
      updateweb = false;
      jsonSysmessage("itho31D9", "failed");
    }
    return;
  }
  else
  {
    if (updateweb)
    {
      updateweb = false;
      jsonSysmessage("itho31D9", i2cbuf2string(i2cbuf, result).c_str());
    }
  }

  auto dataStart = 6;

  if (!ithoInternalMeasurements.empty())
  {
    ithoInternalMeasurements.clear();
  }
  const int labelLen = 4;
  static const char *labels31D9[labelLen] = {0};
  for (int i = 0; i < labelLen; i++)
  {
    if (systemConfig.api_normalize == 0)
    {
      labels31D9[i] = itho31D9Labels[i].labelFull;
    }
    else
    {
      labels31D9[i] = itho31D9Labels[i].labelNormalized;
    }
  }

  float tempVal = i2cbuf[1 + dataStart] / 2.0;
  ithoDeviceMeasurements sTemp = {labels31D9[0], ithoDeviceMeasurements::is_float, {.floatval = tempVal}, 1};
  ithoInternalMeasurements.push_back(sTemp);

  int status = 0;
  if (i2cbuf[0 + dataStart] == 0x80)
  {
    status = 1; // fault
  }
  else
  {
    status = 0; // no fault
  }
  ithoInternalMeasurements.push_back({labels31D9[1], ithoDeviceMeasurements::is_int, {.intval = status}, 1});
  if (i2cbuf[0 + dataStart] == 0x40)
  {
    status = 1; // frost cycle active
  }
  else
  {
    status = 0; // frost cycle not active
  }
  ithoInternalMeasurements.push_back({labels31D9[2], ithoDeviceMeasurements::is_int, {.intval = status}, 1});
  if (i2cbuf[0 + dataStart] == 0x20)
  {
    status = 1; // filter dirty
  }
  else
  {
    status = 0; // filter clean
  }
  ithoInternalMeasurements.push_back({labels31D9[3], ithoDeviceMeasurements::is_int, {.intval = status}, 1});
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

  i2c_31d9_done = true;
}
size_t sendQuery31D9(uint8_t *receive_buffer)
{
  uint8_t command[] = {0x82, 0x80, 0x31, 0xD9, 0x04, 0x00, 0xF0};

  return send_i2c_query(&command[0], sizeof(command), &receive_buffer[0], I2C_CMD_QUERY_31D9);
}

void setSettingCE30(uint16_t temporary_temperature, uint16_t fallback_temperature, uint32_t timestamp, bool updateweb)
// Set the outside temperature for WPU devices.
// [00,3E,CE,30,05,08, 63,91,DA,C7, xx,xx, yy,yy, FF]
// from: 00 (broadcast)
// source: 3E
// command: CE30
// type: 5 (update)
// length: 8
// 6391DAC7 => 4 byte unix timestamp: lifetime of temporary_temparture. After timestamp temporary_temp is replaced by fallback_temp.
// xxxx two byte temporary_temp:  0x0539 = 13.37 degrees C.
// yyyy two byte fallback_temp: 0xFC18 = -10 degrees C.
// FF checksum

{
  uint8_t command[] = {0x00, 0x3E, 0xCE, 0x30, 0x05, 0x08, 0x63, 0x91, 0x00, 0x00, 0x7F, 0xFF, 0x7F, 0xFF, 0xFF};

  command[6] = (timestamp >> 24) & 0xFF;
  command[7] = (timestamp >> 16) & 0xFF;
  command[8] = (timestamp >> 8) & 0xFF;
  command[9] = timestamp & 0xFF;

  command[10] = (temporary_temperature >> 8) & 0xFF;
  command[11] = temporary_temperature & 0xFF;
  command[12] = (fallback_temperature >> 8) & 0xFF;
  command[13] = fallback_temperature & 0xFF;

  command[sizeof(command) - 1] = checksum(command, sizeof(command) - 1);

  if (!i2c_sendBytes(command, sizeof(command), I2C_CMD_SET_CE30))
  {
    if (updateweb)
    {
      updateweb = false;
      jsonSysmessage("itho_CE30_result", "failed");
    }
    return;
  }
}

void setSetting4030(uint16_t index, uint8_t datatype, uint16_t value, uint8_t checked, bool updateweb)
{
  uint8_t command[] = {0x82, 0x80, 0x40, 0x30, 0x06, 0x07, 0x01, 0x00, 0x0F, 0x00, 0x01, 0x01, 0x01, 0xFF};

  command[7] = (index >> 8) & 0xFF;
  command[8] = index & 0xFF;

  command[9] = datatype & 0xFF;

  command[10] = (value >> 8) & 0xFF;
  command[11] = value & 0xFF;

  command[12] = checked & 0xFF;

  command[sizeof(command) - 1] = checksum(command, sizeof(command) - 1);

  D_LOG("Sending 4030: %s", i2cbuf2string(command, sizeof(command)).c_str());
  if (updateweb)
    jsonSysmessage("itho_4030_result", i2cbuf2string(command, sizeof(command)).c_str());

  if (!i2c_sendBytes(command, sizeof(command), I2C_CMD_SET_CE30))
  {
    if (updateweb)
    {
      updateweb = false;
      jsonSysmessage("itho_4030_result", "failed");
    }
    return;
  }
}

int32_t *sendQuery2410(uint8_t index, bool updateweb)
{

  static int32_t values[3];
  values[0] = 0;
  values[1] = 0;
  values[2] = 0;

  uint8_t command[] = {0x82, 0x80, 0x24, 0x10, 0x04, 0x13, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0xFF};

  command[23] = index;
  command[sizeof(command) - 1] = checksum(command, sizeof(command) - 1);

  if (!i2c_sendBytes(command, sizeof(command), I2C_CMD_QUERY_2410))
  {
    if (updateweb)
    {
      jsonSysmessage("itho2410", "failed");
    }
    return nullptr;
  }

  uint8_t i2cbuf[512] = {0};
  size_t len = i2c_slave_receive(i2cbuf);
  if (len > 1 && i2cbuf[len - 1] == checksum(i2cbuf, len - 1) && check_i2c_reply(i2cbuf, len, 0x2410))
  {

    uint8_t tempBuf[] = {i2cbuf[9], i2cbuf[8], i2cbuf[7], i2cbuf[6]};
    std::memcpy(&values[0], tempBuf, 4);

    uint8_t tempBuf2[] = {i2cbuf[13], i2cbuf[12], i2cbuf[11], i2cbuf[10]};
    std::memcpy(&values[1], tempBuf2, 4);

    uint8_t tempBuf3[] = {i2cbuf[17], i2cbuf[16], i2cbuf[15], i2cbuf[14]};
    std::memcpy(&values[2], tempBuf3, 4);

    if (ithoSettingsArray == nullptr)
      return nullptr;

    ithoSettingsArray[index].value = values[0];

    ithoSettingsArray[index].is_signed = get_signed_from_datatype(i2cbuf[22]);
    ithoSettingsArray[index].length = get_length_from_datatype(i2cbuf[22]);
    ithoSettingsArray[index].divider = get_divider_from_datatype(i2cbuf[22]);
    // D_LOG("Itho settings");
    // D_LOG("Divider %d", ithoSettingsArray[index].divider);
    // D_LOG("Length %d", ithoSettingsArray[index].length);

    if (ithoSettingsArray[index].divider == 1)
    { // integer value
      ithoSettingsArray[index].type = ithoSettings::is_int;
    }
    else
    {
      ithoSettingsArray[index].type = ithoSettings::is_float;
    }
    // special cases
    if (i2cbuf[22] == 0x5B)
    {
      // legacy itho: 0x5B -> 0x10
      ithoSettingsArray[index].type = ithoSettings::is_int;
      ;
      ithoSettingsArray[index].length = 2;
      ithoSettingsArray[index].is_signed = false;
    }

    if (updateweb)
    {
      jsonSysmessage("itho2410", i2cbuf2string(i2cbuf, len).c_str());

      char tempbuffer0[256] = {0};
      char tempbuffer1[256] = {0};
      char tempbuffer2[256] = {0};

      int64_t val0, val1, val2;
      val0 = cast_raw_bytes_to_int(&values[0], ithoSettingsArray[index].length, ithoSettingsArray[index].is_signed);
      val1 = cast_raw_bytes_to_int(&values[1], ithoSettingsArray[index].length, ithoSettingsArray[index].is_signed);
      val2 = cast_raw_bytes_to_int(&values[2], ithoSettingsArray[index].length, ithoSettingsArray[index].is_signed);

      if (ithoSettingsArray[index].type == ithoSettings::is_int)
      {
        if (ithoSettingsArray[index].is_signed)
        {
          snprintf(tempbuffer0, sizeof(tempbuffer0), "%lld", val0);
          snprintf(tempbuffer1, sizeof(tempbuffer1), "%lld", val1);
          snprintf(tempbuffer2, sizeof(tempbuffer2), "%lld", val2);
        }
        else
        {
          snprintf(tempbuffer0, sizeof(tempbuffer0), "%llu", val0);
          snprintf(tempbuffer1, sizeof(tempbuffer1), "%llu", val1);
          snprintf(tempbuffer2, sizeof(tempbuffer2), "%llu", val2);
        }
      }
      else
      {
        snprintf(tempbuffer0, sizeof(tempbuffer0), "%.1f", static_cast<double>((int32_t)val0) / ithoSettingsArray[index].divider);
        snprintf(tempbuffer1, sizeof(tempbuffer1), "%.1f", static_cast<double>((int32_t)val1) / ithoSettingsArray[index].divider);
        snprintf(tempbuffer2, sizeof(tempbuffer2), "%.1f", static_cast<double>((int32_t)val2) / ithoSettingsArray[index].divider);
      }
      jsonSysmessage("itho2410cur", tempbuffer0);
      jsonSysmessage("itho2410min", tempbuffer1);
      jsonSysmessage("itho2410max", tempbuffer2);
    }
  }
  else
  {
    D_LOG("itho2410: failed for index:%d", index);
    values[0] = 0x5555AAAA;
    values[1] = 0xAAAA5555;
    values[2] = 0xFFFFFFFF;
    if (updateweb)
    {
      jsonSysmessage("itho2410", "failed");
    }
  }

  return values;
}

bool decodeQuery2410(int32_t *ptr, ithoSettings *setting, double *cur, double *min, double *max)
{
  if (*(ptr + 0) == 0x5555AAAA && *(ptr + 1) == 0xAAAA5555 && *(ptr + 2) == 0xFFFFFFFF)
  {
    return false;
  }

  uint8_t len = setting->length;
  bool is_signed = setting->is_signed;
  int64_t a = cast_raw_bytes_to_int(ptr + 0, len, is_signed);
  int64_t b = cast_raw_bytes_to_int(ptr + 1, len, is_signed);
  int64_t c = cast_raw_bytes_to_int(ptr + 2, len, is_signed);

  *cur = static_cast<double>(static_cast<int32_t>(a));
  *min = static_cast<double>(static_cast<int32_t>(b));
  *max = static_cast<double>(static_cast<int32_t>(c));

  if (setting->type == ithoSettings::is_float)
  {
    *cur = *cur / setting->divider;
    *min = *min / setting->divider;
    *max = *max / setting->divider;
  }

  return true;
}

// void setSetting2410(bool updateweb)
// {
//   uint8_t index = index2410;
//   int32_t value = value2410;
//   setSetting2410(index, value, updateweb);
// }

bool setSetting2410(uint8_t index, int32_t value, bool updateweb)
{

  if (index == 7 && value == 1 && (currentIthoDeviceID() == 0x14 || currentIthoDeviceID() == 0x1B || currentIthoDeviceID() == 0x1D))
  {
    logMessagejson("<br>!!Warning!! Command ignored!<br>Setting index 7 to value 1 will switch off I2C!", WEBINTERFACE);
    return false;
  }

  if (ithoSettingsArray == nullptr)
    return false;

  uint8_t command[] = {0x82, 0x80, 0x24, 0x10, 0x06, 0x13, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0xFF};

  command[23] = index;

  if (ithoSettingsArray[index].length == 1)
  {
    command[9] = value & 0xFF;
  }
  else if (ithoSettingsArray[index].length == 2)
  {
    command[9] = value & 0xFF;
    command[8] = (value >> 8) & 0xFF;
  }
  else if (ithoSettingsArray[index].length == 4)
  {
    command[9] = value & 0xFF;
    command[8] = (value >> 8) & 0xFF;
    command[7] = (value >> 16) & 0xFF;
    command[6] = (value >> 24) & 0xFF;
  }
  else
  {
    if (updateweb)
    {
      jsonSysmessage("itho2410setres", "format error, first use query 2410");
    }
    return false; // unsupported value format
  }

  command[sizeof(command) - 1] = checksum(command, sizeof(command) - 1);

  jsonSysmessage("itho2410set", i2cbuf2string(command, sizeof(command)).c_str());

  if (!i2c_sendBytes(command, sizeof(command), I2C_CMD_SET_2410))
  {
    if (updateweb)
    {
      jsonSysmessage("itho2410setres", "failed");
    }
    return false;
  }

  uint8_t i2cbuf[512] = {0};
  size_t len = i2c_slave_receive(i2cbuf);
  if (len > 1 && i2cbuf[len - 1] == checksum(i2cbuf, len - 1))
  {
    if (updateweb)
    {
      if (len > 2)
      {
        if (command[6] == i2cbuf[6] && command[7] == i2cbuf[7] && command[8] == i2cbuf[8] && command[9] == i2cbuf[9] && command[23] == i2cbuf[23])
        {
          jsonSysmessage("itho2410setres", "confirmed");
          return true;
        }
        else
        {
          jsonSysmessage("itho2410setres", "confirmation failed");
        }
      }
      else
      {
        jsonSysmessage("itho2410setres", "failed");
      }
    }
  }

  return false;
}

void sendQueryCounters(bool updateweb)
{
  uint8_t command[] = {0x82, 0x80, 0x42, 0x10, 0x04, 0x00, 0xA8};
  uint8_t i2cbuf[512]{};

  size_t result = send_i2c_query(&command[0], sizeof(command), &i2cbuf[0], I2C_CMD_QUERY_STATUS);

  if (!result)
  {
    if (updateweb)
    {
      updateweb = false;
      jsonSysmessage("ithocounters", "send i2c command failed");
    }
    return;
  }
  else
  {
    if (updateweb)
    {
      updateweb = false;
      jsonSysmessage("ithocounters", i2cbuf2string(i2cbuf, result).c_str());
    }
  }

  // i2c reply structure:
  //  [I2C addr ][I2C command   ][len payload ][N values][2byte value] ... [2byte value][chk ]
  //  0x80,0x82,  0xC2,0x10,0x01, 0x35,         0x1A,     0x0D,0x26,   ...  0x0A,0xF8,   0xFF

  int valPos = 7; // position of first 2byte value
  int Nvalues = i2cbuf[6];
  if (Nvalues > ithoWPUCounterLabelLength)
  {
    E_LOG("WPU Counter array too long. Counters not read.");
    return;
  }

  if (!ithoCounters.empty())
  {
    ithoCounters.clear();
  }

  uint16_t val;
  for (int i = 0; i < Nvalues; i++)
  {
    int idx = 2 * i + valPos; // idx: start of value in raw bytes
    // read 2 raw bytes: uint16_t.
    val = i2cbuf[idx + 1];
    val |= (i2cbuf[idx] << 8);

    ithoCounters.push_back(ithoDeviceMeasurements());

    // label
    if (systemConfig.api_normalize == 0)
    {
      ithoCounters.back().name = ithoWPUCounterLabels[i].labelFull;
    }
    else
    {
      ithoCounters.back().name = ithoWPUCounterLabels[i].labelNormalized;
    }

    ithoCounters.back().type = ithoDeviceMeasurements::is_int;
    ithoCounters.back().value.intval = val;
  }
}

void filterReset()
{

  //[I2C addr ][  I2C command   ][len ][    timestamp         ][fmt ][    remote ID   ][cntr][cmd opcode][len ][  command ][  counter ][chk]
  uint8_t command[] = {0x82, 0x62, 0xC1, 0x01, 0x01, 0x10, 0xFF, 0xFF, 0xFF, 0xFF, 0x16, 0xFF, 0xFF, 0xFF, 0xFF, 0x10, 0xD0, 0x02, 0x63, 0xFF, 0x00, 0x00, 0xFF};

  unsigned long curtime = millis();

  command[6] = (curtime >> 24) & 0xFF;
  command[7] = (curtime >> 16) & 0xFF;
  command[8] = (curtime >> 8) & 0xFF;
  command[9] = curtime & 0xFF;

  uint8_t id[3]{};
  virtualRemotes.getRemoteIDbyIndex(0, &id[0]);
  command[11] = id[0];
  command[12] = id[1];
  command[13] = id[2];

  command[14] = cmdCounter;
  cmdCounter++;

  command[sizeof(command) - 1] = checksum(command, sizeof(command) - 1);

  i2c_sendBytes(command, sizeof(command), I2C_CMD_FILTER_RESET);
}

void IthoPWMcommand(uint16_t value, volatile uint16_t *ithoCurrentVal, bool *updateIthoMQTT)
{
  uint16_t valTemp = *ithoCurrentVal;
  *ithoCurrentVal = value;

  if (systemConfig.itho_forcemedium)
  {
    IthoCommand precmd = IthoCommand::IthoMedium;
    const RemoteTypes remoteType = virtualRemotes.getRemoteType(0);
    if (remoteType == RemoteTypes::RFTAUTO) // RFT AUTO remote has no meduim button
      precmd = IthoCommand::IthoAuto;
    if (remoteType == RemoteTypes::RFTCVE || remoteType == RemoteTypes::RFTCO2 || remoteType == RemoteTypes::RFTRV) // only handle remotes with mediom command support
      sendRemoteCmd(0, precmd);
  }

  uint8_t command[] = {0x00, 0x60, 0xC0, 0x20, 0x01, 0x02, 0xFF, 0x00, 0xFF};

  uint8_t b = static_cast<uint8_t>(value);

  command[6] = b;
  // command[8] = 0 - (67 + b);
  command[sizeof(command) - 1] = checksum(command, sizeof(command) - 1);

  if (i2c_sendBytes(command, sizeof(command), I2C_CMD_PWM_CMD))
  {
    *updateIthoMQTT = true;
  }
  else
  {
    *ithoCurrentVal = valTemp;
    ithoQueue.add2queue(valTemp, 0, systemConfig.nonQ_cmd_clearsQ);
  }
}

size_t send_i2c_query(uint8_t *i2c_command, size_t i2c_command_length, uint8_t *receive_buffer, i2c_cmdref_t origin)
{
  if (i2c_command == nullptr)
    return 0;
  if (receive_buffer == nullptr)
    return 0;
  if (!i2c_sendBytes(i2c_command, i2c_command_length, origin))
    return 0;

  size_t len = i2c_slave_receive(receive_buffer);
  if (!len)
  {
    return 0;
  }

  uint16_t opcode = (i2c_command[2] << 8 | i2c_command[3]);
  if (len > 1 && receive_buffer[len - 1] == checksum(receive_buffer, len - 1) && check_i2c_reply(receive_buffer, len, opcode))
  {
    return len;
  }
  return 0;
}
int quick_pow10(int n)
{
  static int pow10[10] = {
      1, 10, 100, 1000, 10000,
      100000, 1000000, 10000000, 100000000, 1000000000};
  if (n > (sizeof(pow10) / sizeof(pow10[0]) - 1))
    return 1;
  return pow10[n];
}

std::string i2cbuf2string(const uint8_t *data, size_t len)
{
  std::string s;
  s.reserve(len * 3 + 2);
  for (size_t i = 0; i < len; ++i)
  {
    if (i)
      s += ' ';
    s += toHex(data[i] >> 4);
    s += toHex(data[i] & 0xF);
  }
  return s;
}

bool check_i2c_reply(const uint8_t *buf, size_t buflen, const uint16_t opcode)
{
  if (buflen < 4)
    return false;

  return ((buf[2] << 8 | buf[3]) & 0x3FFF) == (opcode & 0x3FFF);
}
int cast_to_signed_int(int val, int length)
{
  switch (length)
  {
  case 4:
    return static_cast<int32_t>(val);
  case 2:
    return static_cast<int16_t>(val);
  case 1:
    return static_cast<int8_t>(val);
  default:
    return 0;
  }
}

int64_t cast_raw_bytes_to_int(int32_t *valptr, int length, bool is_signed)
// valptr is a pointer to 4 rawbytes, which are casted to the
//  correct value and returned as int32.
{
  if (is_signed)
  {
    switch (length)
    {
    case 4:
      return *reinterpret_cast<int32_t *>(valptr);
    case 2:
      return *reinterpret_cast<int16_t *>(valptr);
    case 1:
      return *reinterpret_cast<int8_t *>(valptr);
    default:
      return 0;
    }
  }
  else
  {
    switch (length)
    {
    case 4:
      return *reinterpret_cast<uint32_t *>(valptr);
    case 2:
      return *reinterpret_cast<uint16_t *>(valptr);
    case 1:
      return *reinterpret_cast<uint8_t *>(valptr);
    default:
      return 0;
    }
  }
}

// Itho datatype
// bit 7: signed (1), unsigned (0)
// bit 6,5,4 : length
// bit 3,2,1,0 : divider
uint32_t get_divider_from_datatype(int8_t datatype)
{

  const uint32_t _divider[] =
      {
          1, 10, 100, 1000, 10000, 100000,
          1000000, 10000000, 100000000,
          1, 1, // dividers index 9 and 10 should be 0.1 and 0.01
          1, 1, 1, 256, 2};
  return _divider[datatype & 0x0f];
}

uint8_t get_length_from_datatype(int8_t datatype)
{

  switch (datatype & 0x70)
  {
  case 0x10:
    return 2;
  case 0x20:
  case 0x70:
    return 4;
  default:
    return 1;
  }
}

bool get_signed_from_datatype(int8_t datatype)
{
  return datatype & 0x80;
}
