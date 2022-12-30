
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
uint8_t itho_fwversion = 0;
volatile uint16_t ithoCurrentVal = 0;
const struct ihtoDeviceType *ithoDeviceptr = nullptr;
int16_t ithoSettingsLength = 0;
int16_t ithoStatusLabelLength = 0;
std::vector<ithoDeviceStatus> ithoStatus;
std::vector<ithoDeviceMeasurements> ithoMeasurements;
std::vector<ithoDeviceMeasurements> ithoInternalMeasurements;
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

bool itho_internal_hum_temp = false;
double ithoHum = 0;
double ithoTemp = 0;

//                                  { IthoUnknown,  IthoJoin, IthoLeave,  IthoAway, IthoLow, IthoMedium,  IthoHigh,  IthoFull, IthoTimer1,  IthoTimer2,  IthoTimer3,  IthoAuto,  IthoAutoNight, IthoCook30,  IthoCook60 }
const uint8_t *RFTCVE_Remote_Map[] = {nullptr, ithoMessageCVERFTJoinCommandBytes, ithoMessageLeaveCommandBytes, ithoMessageAwayCommandBytes, ithoMessageLowCommandBytes, ithoMessageMediumCommandBytes, ithoMessageHighCommandBytes, nullptr, ithoMessageTimer1CommandBytes, ithoMessageTimer2CommandBytes, ithoMessageTimer3CommandBytes, nullptr, nullptr, nullptr, nullptr};
const uint8_t *RFTAUTO_Remote_Map[] = {nullptr, ithoMessageAUTORFTJoinCommandBytes, ithoMessageAUTORFTLeaveCommandBytes, nullptr, ithoMessageAUTORFTLowCommandBytes, nullptr, ithoMessageAUTORFTHighCommandBytes, nullptr, ithoMessageAUTORFTTimer1CommandBytes, ithoMessageAUTORFTTimer2CommandBytes, ithoMessageAUTORFTTimer3CommandBytes, ithoMessageAUTORFTAutoCommandBytes, ithoMessageAUTORFTAutoNightCommandBytes, nullptr, nullptr};
const uint8_t *DEMANDFLOW_Remote_Map[] = {nullptr, ithoMessageDFJoinCommandBytes, ithoMessageLeaveCommandBytes, nullptr, ithoMessageDFLowCommandBytes, nullptr, ithoMessageDFHighCommandBytes, nullptr, ithoMessageDFTimer1CommandBytes, ithoMessageDFTimer2CommandBytes, ithoMessageDFTimer3CommandBytes, nullptr, nullptr, ithoMessageDFCook30CommandBytes, ithoMessageDFCook60CommandBytes};
const uint8_t *RFTRV_Remote_Map[] = {nullptr, ithoMessageRVJoinCommandBytes, ithoMessageLeaveCommandBytes, nullptr, ithoMessageLowCommandBytes, ithoMessageRV_CO2MediumCommandBytes, ithoMessageHighCommandBytes, nullptr, ithoMessageRV_CO2Timer1CommandBytes, ithoMessageRV_CO2Timer2CommandBytes, ithoMessageRV_CO2Timer3CommandBytes, ithoMessageRV_CO2AutoCommandBytes, ithoMessageRV_CO2AutoNightCommandBytes, nullptr, nullptr};
const uint8_t *RFTCO2_Remote_Map[] = {nullptr, ithoMessageCO2JoinCommandBytes, ithoMessageLeaveCommandBytes, nullptr, ithoMessageLowCommandBytes, ithoMessageRV_CO2MediumCommandBytes, ithoMessageHighCommandBytes, nullptr, ithoMessageRV_CO2Timer1CommandBytes, ithoMessageRV_CO2Timer2CommandBytes, ithoMessageRV_CO2Timer3CommandBytes, ithoMessageRV_CO2AutoCommandBytes, ithoMessageRV_CO2AutoNightCommandBytes, nullptr, nullptr};

struct ihtoRemoteCmdMap
{
  RemoteTypes type;
  const uint8_t **cammandMapping;
};

const struct ihtoRemoteCmdMap ihtoRemoteCmdMapping[]
{
  {RFTCVE, RFTCVE_Remote_Map},
      {RFTAUTO, RFTAUTO_Remote_Map},
      {DEMANDFLOW, DEMANDFLOW_Remote_Map},
      {RFTRV, RFTRV_Remote_Map},
  {
    RFTCO2, RFTCO2_Remote_Map
  }
};

int currentIthoDeviceGroup() { return ithoDeviceGroup; }
int currentIthoDeviceID() { return ithoDeviceID; }
uint8_t currentItho_fwversion() { return itho_fwversion; }
uint16_t currentIthoSettingsLength() { return ithoSettingsLength; }
int16_t currentIthoStatusLabelLength() { return ithoStatusLabelLength; }
const uint8_t *getRemoteCmd(const RemoteTypes type, const IthoCommand command)
{

  const struct ihtoRemoteCmdMap *ihtoRemoteCmdMapPtr = ihtoRemoteCmdMapping;
  const struct ihtoRemoteCmdMap *ihtoRemoteCmdMapEndPtr = ihtoRemoteCmdMapping + sizeof(ihtoRemoteCmdMapping) / sizeof(ihtoRemoteCmdMapping[0]);
  while (ihtoRemoteCmdMapPtr < ihtoRemoteCmdMapEndPtr)
  {
    if (ihtoRemoteCmdMapPtr->type == type)
    {
      return *(ihtoRemoteCmdMapPtr->cammandMapping + command);
    }
    ihtoRemoteCmdMapPtr++;
  }
  return nullptr;
}

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

const struct ihtoDeviceType ithoDevices[]
{
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
  {
    0x07, 0x01, "HRU 250-300", ithoHRU250_300SettingsMap, sizeof(ithoHRU250_300SettingsMap) / sizeof(ithoHRU250_300SettingsMap[0]), ithoHRU250_300SettingsLabels, ithoHRU250_300StatusMap, sizeof(ithoHRU250_300StatusMap) / sizeof(ithoHRU250_300StatusMap[0]), ithoHRU250_300StatusLabels
  }
};

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
        return -3; // Settings not available for this version
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

  const struct ihtoDeviceType *settingsPtr = ithoDeviceptr;

  StaticJsonDocument<512> doc;
  JsonObject root = doc.to<JsonObject>();

  root["Index"] = index;
  root["Description"] = settingsPtr->settingsDescriptions[static_cast<int>(*(*(settingsPtr->settingsMapping + version) + index))];

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
    i2c_cmd_queue.push_back([index]()
                            { resultPtr2410 = sendQuery2410(index, false); });
    i2c_cmd_queue.push_back([index, loop]()
                            { processSettingResult(index, loop); });
  }
  else // -> update setting values from other source (ie. debug page)
  {
    // index2410 = i;
    // i2c_result_updateweb = updateweb;
    resultPtr2410 = nullptr;
    // get2410 = true;
    i2c_cmd_queue.push_back([index, updateweb]()
                            { resultPtr2410 = sendQuery2410(index, updateweb); });
  }
}

void processSettingResult(const uint8_t index, const bool loop)
{

  const uint8_t version = currentItho_fwversion();

  const struct ihtoDeviceType *settingsPtr = ithoDeviceptr;

  StaticJsonDocument<512> doc;
  JsonObject root = doc.to<JsonObject>();

  root["Index"] = index;
  root["Description"] = settingsPtr->settingsDescriptions[static_cast<int>(*(*(settingsPtr->settingsMapping + version) + index))];

  auto timeoutmillis = millis() + 3000; // about 1 sec. + 2 sec. for potential i2c queue pause
  while (resultPtr2410 == nullptr && millis() < timeoutmillis)
  {
    // wait for result
  }
  root["update"] = true;
  root["loop"] = loop;
  if (resultPtr2410 != nullptr && ithoSettingsArray != nullptr)
  {
    if (ithoSettingsArray[index].type == ithoSettings::is_int8)
    {
      root["Current"] = *(reinterpret_cast<int8_t *>(resultPtr2410 + 0));
      root["Minimum"] = *(reinterpret_cast<int8_t *>(resultPtr2410 + 1));
      root["Maximum"] = *(reinterpret_cast<int8_t *>(resultPtr2410 + 2));
    }
    else if (ithoSettingsArray[index].type == ithoSettings::is_int16)
    {
      root["Current"] = *(reinterpret_cast<int16_t *>(resultPtr2410 + 0));
      root["Minimum"] = *(reinterpret_cast<int16_t *>(resultPtr2410 + 1));
      root["Maximum"] = *(reinterpret_cast<int16_t *>(resultPtr2410 + 2));
    }
    else if (ithoSettingsArray[index].type == ithoSettings::is_uint8 || ithoSettingsArray[index].type == ithoSettings::is_uint16 || ithoSettingsArray[index].type == ithoSettings::is_uint32)
    {
      root["Current"] = *(reinterpret_cast<uint32_t *>(resultPtr2410 + 0));
      root["Minimum"] = *(reinterpret_cast<uint32_t *>(resultPtr2410 + 1));
      root["Maximum"] = *(reinterpret_cast<uint32_t *>(resultPtr2410 + 2));
    }
    else if (ithoSettingsArray[index].type == ithoSettings::is_float2)
    {
      root["Current"] = *(reinterpret_cast<int32_t *>(resultPtr2410 + 0)) / 2.0;
      root["Minimum"] = *(reinterpret_cast<int32_t *>(resultPtr2410 + 1)) / 2.0;
      root["Maximum"] = *(reinterpret_cast<int32_t *>(resultPtr2410 + 2)) / 2.0;
    }
    else if (ithoSettingsArray[index].type == ithoSettings::is_float10)
    {
      root["Current"] = *(reinterpret_cast<int32_t *>(resultPtr2410 + 0)) / 10.0;
      root["Minimum"] = *(reinterpret_cast<int32_t *>(resultPtr2410 + 1)) / 10.0;
      root["Maximum"] = *(reinterpret_cast<int32_t *>(resultPtr2410 + 2)) / 10.0;
    }
    else if (ithoSettingsArray[index].type == ithoSettings::is_float100)
    {
      root["Current"] = *(reinterpret_cast<int32_t *>(resultPtr2410 + 0)) / 100.0;
      root["Minimum"] = *(reinterpret_cast<int32_t *>(resultPtr2410 + 1)) / 100.0;
      root["Maximum"] = *(reinterpret_cast<int32_t *>(resultPtr2410 + 2)) / 100.0;
    }
    else if (ithoSettingsArray[index].type == ithoSettings::is_float1000)
    {
      root["Current"] = *(reinterpret_cast<int32_t *>(resultPtr2410 + 0)) / 1000.0;
      root["Minimum"] = *(reinterpret_cast<int32_t *>(resultPtr2410 + 1)) / 1000.0;
      root["Maximum"] = *(reinterpret_cast<int32_t *>(resultPtr2410 + 2)) / 1000.0;
    }
    else
    {
      root["Current"] = *(resultPtr2410 + 0);
      root["Minimum"] = *(resultPtr2410 + 1);
      root["Maximum"] = *(resultPtr2410 + 2);
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

int getStatusLabelLength(const uint8_t deviceID, const uint8_t version)
{

  const struct ihtoDeviceType *ithoDevicesptr = ithoDevices;
  const struct ihtoDeviceType *ithoDevicesendPtr = ithoDevices + sizeof(ithoDevices) / sizeof(ithoDevices[0]);
  while (ithoDevicesptr < ithoDevicesendPtr)
  {
    if (ithoDevicesptr->ID == deviceID)
    {
      if (ithoDevicesptr->statusLabelMapping == nullptr)
      {
        return -2; // Labels not available for this device
      }
      if (version > (ithoDevicesptr->statusMapLen - 1))
      {
        return -3; // Labels not available for this version
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

const char *getSatusLabel(const uint8_t i, const struct ihtoDeviceType *statusPtr, const uint8_t version)
{

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
  else if (currentIthoStatusLabelLength() == -2)
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
  else if (currentIthoStatusLabelLength() == -3)
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
  else if (!(i < currentIthoStatusLabelLength()))
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

  i2c_cmd_queue.push_back([index, value, webupdate]()
                          { setSetting2410(index, value, webupdate); });

  i2c_cmd_queue.push_back([index]()
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

  uint8_t command[] = {0x82, 0xEF, 0xC0, 0x00, 0x01, 0x06, 0x00, 0x00, 0x09, 0x00, 0x09, 0x00, 0xB6};

  command[sizeof(command) - 1] = checksum(command, sizeof(command) - 1);

  i2c_sendBytes(command, sizeof(command), I2C_CMD_PWM_INIT);
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
  const uint8_t *remote_command = getRemoteCmd(remoteType, command);
  if (remote_command == nullptr)
    return;

  uint8_t i2c_command[64] = {};
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

  const int *id = virtualRemotes.getRemoteIDbyIndex(remoteIndex);
  i2c_header[11] = *id;
  i2c_header[12] = *(id + 1);
  i2c_header[13] = *(id + 2);

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
    i2c_command_len++;
  }

  // if join or leave, add remote ID fields
  if (command == IthoJoin || command == IthoLeave)
  {
    // set command ID's
    if (command_len > 0x05)
    {
      // add 1st ID
      i2c_command[21] = *id;
      i2c_command[22] = *(id + 1);
      i2c_command[23] = *(id + 2);
    }
    if (command_len > 0x0B)
    {
      // add 2nd ID
      i2c_command[27] = *id;
      i2c_command[28] = *(id + 1);
      i2c_command[29] = *(id + 2);
    }
    if (command_len > 0x12)
    {
      // add 3rd ID
      i2c_command[33] = *id;
      i2c_command[34] = *(id + 1);
      i2c_command[35] = *(id + 2);
    }
    if (command_len > 0x17)
    {
      // add 4th ID
      i2c_command[39] = *id;
      i2c_command[40] = *(id + 1);
      i2c_command[41] = *(id + 2);
    }
    if (command_len > 0x1D)
    {
      // add 5th ID
      i2c_command[45] = *id;
      i2c_command[46] = *(id + 1);
      i2c_command[47] = *(id + 2);
    }
  }

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
  uint8_t i2c_command_tmp[64] = {};
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
  char buf[250]{};
  for (int i = 0; i < i2c_command_len; i++)
  {
    sprintf(buf, " 0x%02X", i2c_command[i]);
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

  if (!i2c_sendBytes(command, sizeof(command), I2C_CMD_QUERY_DEVICE_TYPE))
  {
    if (updateweb)
    {
      jsonSysmessage("ithotype", "failed");
    }
  }

  uint8_t i2cbuf[512]{};
  size_t len = i2c_slave_receive(i2cbuf);

  // if (len > 2) {
  if (len > 1 && i2cbuf[len - 1] == checksum(i2cbuf, len - 1))
  {
    if (updateweb)
    {
      jsonSysmessage("ithotype", i2cbuf2string(i2cbuf, len).c_str());
    }

    ithoDeviceGroup = i2cbuf[8];
    ithoDeviceID = i2cbuf[9];
    itho_fwversion = i2cbuf[11];
    ithoDeviceptr = getDevicePtr(currentIthoDeviceGroup(), currentIthoDeviceID());
    ithoSettingsLength = getSettingsLength(currentIthoDeviceGroup(), currentIthoDeviceID(), currentItho_fwversion());
    restest = true;
  }
  else
  {
    if (updateweb)
    {
      jsonSysmessage("ithotype", "failed");
    }
  }
}

void sendQueryStatusFormat(bool updateweb)
{

  uint8_t command[] = {0x82, 0x80, 0x24, 0x00, 0x04, 0x00, 0xD6};

  if (!i2c_sendBytes(command, sizeof(command), I2C_CMD_QUERY_STATUS_FORMAT))
  {
    if (updateweb)
    {
      updateweb = false;
      jsonSysmessage("ithostatusformat", "failed");
    }
    return;
  }

  uint8_t i2cbuf[512]{};
  uint8_t len = i2c_slave_receive(i2cbuf);
  if (len > 1 && i2cbuf[len - 1] == checksum(i2cbuf, len - 1))
  {
    if (updateweb)
    {
      updateweb = false;
      jsonSysmessage("ithostatusformat", i2cbuf2string(i2cbuf, len).c_str());
    }

    if (!ithoStatus.empty())
    {
      ithoStatus.clear();
    }
    if (!(currentItho_fwversion() > 0))
      return;
    ithoStatusLabelLength = getStatusLabelLength(currentIthoDeviceID(), currentItho_fwversion());
    const uint8_t endPos = i2cbuf[5];

    for (uint8_t i = 0; i < endPos; i++)
    {
      ithoStatus.push_back(ithoDeviceStatus());

      //      char fStringBuf[32];
      //      getSatusLabel(i, ithoDeviceptr, currentItho_fwversion(), fStringBuf);

      ithoStatus.back().divider = 0;
      if ((i2cbuf[6 + i] & 0x07) == 0)
      { // integer value
        if ((i2cbuf[6 + i] & 0x80) == 0)
        { // unsigned value
          ithoStatus.back().type = ithoDeviceStatus::is_uint;
        }
        else
        {
          ithoStatus.back().type = ithoDeviceStatus::is_int;
        }
      }
      else
      {
        ithoStatus.back().type = ithoDeviceStatus::is_float;
        ithoStatus.back().divider = quick_pow10((i2cbuf[6 + i] & 0x07));
      }
      if (((i2cbuf[6 + i] >> 3) & 0x07) == 0)
      {
        ithoStatus.back().length = 1;
      }
      else
      {
        ithoStatus.back().length = (i2cbuf[6 + i] >> 3) & 0x07;
      }

      // special cases
      if (i2cbuf[6 + i] == 0x0C || i2cbuf[6 + i] == 0x6C)
      {
        ithoStatus.back().type = ithoDeviceStatus::is_byte;
        ithoStatus.back().length = 1;
      }
      if (i2cbuf[6 + i] == 0x0F)
      {
        ithoStatus.back().type = ithoDeviceStatus::is_float;
        ithoStatus.back().length = 1;
        ithoStatus.back().divider = 2;
      }
      if (i2cbuf[6 + i] == 0x5B)
      {
        ithoStatus.back().type = ithoDeviceStatus::is_uint;
        ithoStatus.back().length = 2;
      }
    }
  }
  else
  {
    if (updateweb)
    {
      updateweb = false;
      jsonSysmessage("ithostatusformat", "failed");
    }
  }
}

void sendQueryStatus(bool updateweb)
{

  uint8_t command[] = {0x82, 0x80, 0x24, 0x01, 0x04, 0x00, 0xD5};

  if (!i2c_sendBytes(command, sizeof(command), I2C_CMD_QUERY_STATUS))
  {
    if (updateweb)
    {
      updateweb = false;
      jsonSysmessage("ithostatus", "failed");
    }
    return;
  }

  uint8_t i2cbuf[512]{};
  size_t len = i2c_slave_receive(i2cbuf);

  if (len > 1 && i2cbuf[len - 1] == checksum(i2cbuf, len - 1))
  {

    if (updateweb)
    {
      updateweb = false;
      jsonSysmessage("ithostatus", i2cbuf2string(i2cbuf, len).c_str());
    }

    int statusPos = 6; // first byte with status info
    int labelPos = 0;
    if (!ithoStatus.empty())
    {
      for (auto &ithoStat : ithoStatus)
      {
        ithoStat.name = getSatusLabel(labelPos, ithoDeviceptr, currentItho_fwversion());
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
        if (ithoStat.type == ithoDeviceStatus::is_uint)
        {
          if (ithoStat.value.uintval == tempVal)
          {
            ithoStat.updated = 0;
          }
          else
          {
            ithoStat.updated = 1;
            ithoStat.value.uintval = tempVal;
          }
        }
        if (ithoStat.type == ithoDeviceStatus::is_int)
        {
          if (ithoStat.length == 4)
          {
            tempVal = static_cast<int32_t>(tempVal);
          }
          if (ithoStat.length == 2)
          {
            tempVal = static_cast<int16_t>(tempVal);
          }
          if (ithoStat.length == 1)
          {
            tempVal = static_cast<int8_t>(tempVal);
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
          double t = ithoStat.value.floatval * ithoStat.divider;
          if (static_cast<uint32_t>(t) == tempVal) // better compare needed of float val, worst case this will result in an extra update of the value, so limited impact
          {
            ithoStat.updated = 0;
          }
          else
          {
            ithoStat.updated = 1;
            ithoStat.value.floatval = static_cast<double>(tempVal) / ithoStat.divider;
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
  else
  {
    if (updateweb)
    {
      updateweb = false;
      jsonSysmessage("ithostatus", "failed");
    }
  }
}

void sendQuery31DA(bool updateweb)
{

  uint8_t command[] = {0x82, 0x80, 0x31, 0xDA, 0x04, 0x00, 0xEF};

  if (!i2c_sendBytes(command, sizeof(command), I2C_CMD_QUERY_31DA))
  {
    if (updateweb)
    {
      updateweb = false;
      jsonSysmessage("itho31DA", "failed");
    }
    return;
  }

  uint8_t i2cbuf[512]{};
  size_t len = i2c_slave_receive(i2cbuf);

  if (len > 1 && i2cbuf[len - 1] == checksum(i2cbuf, len - 1))
  {

    if (updateweb)
    {
      updateweb = false;
      jsonSysmessage("itho31DA", i2cbuf2string(i2cbuf, len).c_str());
    }

    auto dataLength = i2cbuf[5];

    auto dataStart = 6;
    if (!ithoMeasurements.empty())
    {
      ithoMeasurements.clear();
    }
    const int labelLen = 19;
    static const char *labels31DA[labelLen]{};
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
  else
  {
    if (updateweb)
    {
      updateweb = false;
      jsonSysmessage("itho31DA", "failed");
    }
  }
}

void sendQuery31D9(bool updateweb)
{

  uint8_t command[] = {0x82, 0x80, 0x31, 0xD9, 0x04, 0x00, 0xF0};

  if (!i2c_sendBytes(command, sizeof(command), I2C_CMD_QUERY_31D9))
  {
    if (updateweb)
    {
      updateweb = false;
      jsonSysmessage("itho31D9", "failed");
    }
    return;
  }

  uint8_t i2cbuf[512]{};
  size_t len = i2c_slave_receive(i2cbuf);
  if (len > 1 && i2cbuf[len - 1] == checksum(i2cbuf, len - 1))
  {

    if (updateweb)
    {
      updateweb = false;
      jsonSysmessage("itho31D9", i2cbuf2string(i2cbuf, len).c_str());
    }

    auto dataStart = 6;

    if (!ithoInternalMeasurements.empty())
    {
      ithoInternalMeasurements.clear();
    }
    const int labelLen = 4;
    static const char *labels31D9[labelLen]{};
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

    double tempVal = i2cbuf[1 + dataStart] / 2.0;
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
  }
  else
  {
    if (updateweb)
    {
      updateweb = false;
      jsonSysmessage("itho31D9", "failed");
    }
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

  uint8_t i2cbuf[512]{};
  size_t len = i2c_slave_receive(i2cbuf);
  if (len > 1 && i2cbuf[len - 1] == checksum(i2cbuf, len - 1))
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

    if (((i2cbuf[22] >> 3) & 0x07) == 0)
    {
      ithoSettingsArray[index].length = 1;
    }
    else
    {
      ithoSettingsArray[index].length = (i2cbuf[22] >> 3) & 0x07;
    }

    if ((i2cbuf[22] & 0x07) == 0)
    { // integer value
      if ((i2cbuf[22] & 0x80) == 0)
      { // unsigned value
        if (ithoSettingsArray[index].length == 1)
        {
          ithoSettingsArray[index].type = ithoSettings::is_uint8;
        }
        else if (ithoSettingsArray[index].length == 2)
        {
          ithoSettingsArray[index].type = ithoSettings::is_uint16;
        }
        else
        {
          ithoSettingsArray[index].type = ithoSettings::is_uint32;
        }
      }
      else
      {
        if (ithoSettingsArray[index].length == 1)
        {
          ithoSettingsArray[index].type = ithoSettings::is_int8;
        }
        else if (ithoSettingsArray[index].length == 2)
        {
          ithoSettingsArray[index].type = ithoSettings::is_int16;
        }
        else
        {
          ithoSettingsArray[index].type = ithoSettings::is_int32;
        }
      }
    }
    else
    { // float

      if (i2cbuf[22] == 0x0f)
      {
        ithoSettingsArray[index].type = ithoSettings::is_float2;
      }
      else if ((i2cbuf[22] & 0x04) != 0)
      {
        ithoSettingsArray[index].type = ithoSettings::is_float1000;
      }
      else if ((i2cbuf[22] & 0x02) != 0)
      {
        ithoSettingsArray[index].type = ithoSettings::is_float100;
      }
      else if ((i2cbuf[22] & 0x01) != 0)
      {
        ithoSettingsArray[index].type = ithoSettings::is_float10;
      }
      else
      {
        ithoSettingsArray[index].type = ithoSettings::is_unknown;
      }
    }
    
    if (updateweb)
    {
      jsonSysmessage("itho2410", i2cbuf2string(i2cbuf, len).c_str());

      char tempbuffer0[256]{};
      char tempbuffer1[256]{};
      char tempbuffer2[256]{};

      if (ithoSettingsArray[index].type == ithoSettings::is_uint8 || ithoSettingsArray[index].type == ithoSettings::is_uint16 || ithoSettingsArray[index].type == ithoSettings::is_uint32)
      { // unsigned value
        uint32_t val[3];
        std::memcpy(&val[0], &values[0], sizeof(val[0]));
        std::memcpy(&val[1], &values[1], sizeof(val[0]));
        std::memcpy(&val[2], &values[2], sizeof(val[0]));
        sprintf(tempbuffer0, "%u", val[0]);
        sprintf(tempbuffer1, "%u", val[1]);
        sprintf(tempbuffer2, "%u", val[2]);
      }
      else if (ithoSettingsArray[index].type == ithoSettings::is_int8)
      {
        int8_t val[3];
        std::memcpy(&val[0], &values[0], sizeof(val[0]));
        std::memcpy(&val[1], &values[1], sizeof(val[0]));
        std::memcpy(&val[2], &values[2], sizeof(val[0]));
        sprintf(tempbuffer0, "%d", val[0]);
        sprintf(tempbuffer1, "%d", val[1]);
        sprintf(tempbuffer2, "%d", val[2]);
      }
      else if (ithoSettingsArray[index].type == ithoSettings::is_int16)
      {
        int16_t val[3];
        std::memcpy(&val[0], &values[0], sizeof(val[0]));
        std::memcpy(&val[1], &values[1], sizeof(val[0]));
        std::memcpy(&val[2], &values[2], sizeof(val[0]));
        sprintf(tempbuffer0, "%d", val[0]);
        sprintf(tempbuffer1, "%d", val[1]);
        sprintf(tempbuffer2, "%d", val[2]);
      }
      else if (ithoSettingsArray[index].type == ithoSettings::is_int32)
      {
        sprintf(tempbuffer0, "%d", values[0]);
        sprintf(tempbuffer1, "%d", values[1]);
        sprintf(tempbuffer2, "%d", values[2]);
      }
      else if (ithoSettingsArray[index].type == ithoSettings::is_float2)
      {
        double val[3];
        val[0] = values[0] / 2.0f;
        val[1] = values[1] / 2.0f;
        val[2] = values[2] / 2.0f;
        sprintf(tempbuffer0, "%.1f", val[0]);
        sprintf(tempbuffer1, "%.1f", val[1]);
        sprintf(tempbuffer2, "%.1f", val[2]);
      }
      else if (ithoSettingsArray[index].type == ithoSettings::is_float10)
      {
        double val[3];
        val[0] = values[0] / 10.0f;
        val[1] = values[1] / 10.0f;
        val[2] = values[2] / 10.0f;
        sprintf(tempbuffer0, "%.1f", val[0]);
        sprintf(tempbuffer1, "%.1f", val[1]);
        sprintf(tempbuffer2, "%.1f", val[2]);
      }
      else if (ithoSettingsArray[index].type == ithoSettings::is_float100)
      {
        double val[3];
        val[0] = values[0] / 100.0f;
        val[1] = values[1] / 100.0f;
        val[2] = values[2] / 100.0f;
        sprintf(tempbuffer0, "%.2f", val[0]);
        sprintf(tempbuffer1, "%.2f", val[1]);
        sprintf(tempbuffer2, "%.2f", val[2]);
      }
      else if (ithoSettingsArray[index].type == ithoSettings::is_float1000)
      {
        double val[3];
        val[0] = values[0] / 1000.0f;
        val[1] = values[1] / 1000.0f;
        val[2] = values[2] / 1000.0f;
        sprintf(tempbuffer0, "%.3f", val[0]);
        sprintf(tempbuffer1, "%.3f", val[1]);
        sprintf(tempbuffer2, "%.3f", val[2]);
      }
      else
      {
        sprintf(tempbuffer0, "%d", values[0]);
        sprintf(tempbuffer1, "%d", values[1]);
        sprintf(tempbuffer2, "%d", values[2]);
      }
      jsonSysmessage("itho2410cur", tempbuffer0);
      jsonSysmessage("itho2410min", tempbuffer1);
      jsonSysmessage("itho2410max", tempbuffer2);
    }
  }
  else
  {
    if (updateweb)
    {
      jsonSysmessage("itho2410", "failed");
    }
  }

  return values;
}

// void setSetting2410(bool updateweb)
// {
//   uint8_t index = index2410;
//   int32_t value = value2410;
//   setSetting2410(index, value, updateweb);
// }

void setSetting2410(uint8_t index, int32_t value, bool updateweb)
{

  if (index == 7 && value == 1 && (currentIthoDeviceID() == 0x14 || currentIthoDeviceID() == 0x1B || currentIthoDeviceID() == 0x1D))
  {
    logMessagejson("<br>!!Warning!! Command ignored!<br>Setting index 7 to value 1 will switch off I2C!", WEBINTERFACE);
    return;
  }

  if (ithoSettingsArray == nullptr)
    return;

  uint8_t command[] = {0x82, 0x80, 0x24, 0x10, 0x06, 0x13, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0xFF};

  command[23] = index;

  if (ithoSettingsArray[index].type == ithoSettings::is_uint8 || ithoSettingsArray[index].type == ithoSettings::is_int8)
  {
    command[9] = value & 0xFF;
  }
  else if (ithoSettingsArray[index].type == ithoSettings::is_uint16 || ithoSettingsArray[index].type == ithoSettings::is_int16)
  {
    command[9] = value & 0xFF;
    command[8] = (value >> 8) & 0xFF;
  }
  else if (ithoSettingsArray[index].type == ithoSettings::is_uint32 || ithoSettingsArray[index].type == ithoSettings::is_int32 || ithoSettingsArray[index].type == ithoSettings::is_float2 || ithoSettingsArray[index].type == ithoSettings::is_float10 || ithoSettingsArray[index].type == ithoSettings::is_float100 || ithoSettingsArray[index].type == ithoSettings::is_float1000)
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
    return; // unsupported value format
  }

  command[sizeof(command) - 1] = checksum(command, sizeof(command) - 1);

  jsonSysmessage("itho2410set", i2cbuf2string(command, sizeof(command)).c_str());

  if (!i2c_sendBytes(command, sizeof(command), I2C_CMD_SET_2410))
  {
    if (updateweb)
    {
      jsonSysmessage("itho2410setres", "failed");
    }
    return;
  }

  uint8_t i2cbuf[512]{};
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

  const int *id = virtualRemotes.getRemoteIDbyIndex(0);
  command[11] = *id;
  command[12] = *(id + 1);
  command[13] = *(id + 2);

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
