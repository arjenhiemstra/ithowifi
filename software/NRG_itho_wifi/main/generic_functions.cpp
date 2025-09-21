#include "generic_functions.h"

#define MAX_FIRMWARE_HTTPS_RESPONSE_SIZE 2000 // firmware json response should be smaller than this

// locals
Ticker IthoCMD;
const char *espName = "nrg-itho-";

volatile uint16_t nextIthoVal = 0;
volatile unsigned long nextIthoTimer = 0;

const char *hostName()
{
  static char hostName[32]{};

  if (strcmp(wifiConfig.hostname, "") == 0)
  {
    snprintf(hostName, sizeof(hostName), "%s%02x%02x", espName, sys.getMac(4), sys.getMac(5));
  }
  else
  {
    strlcpy(hostName, wifiConfig.hostname, sizeof(hostName));
  }

  return hostName;
}

uint8_t getIthoStatusJSON(JsonObject root)
{
  uint8_t index = 0;
  if (SHT3x_original || SHT3x_alternative || itho_internal_hum_temp)
  {
    root["temp"] = round(ithoTemp, 1);
    index++;
    root["hum"] = round(ithoHum, 1);
    index++;
    auto b = 611.21 * pow(2.7183, ((18.678 - ithoTemp / 234.5) * ithoTemp) / (257.14 + ithoTemp));
    auto ppmw = b / (101325 - b) * ithoHum / 100 * 0.62145 * 1000000;
    root["ppmw"] = static_cast<int>(ppmw + 0.5);
    index++;
  }
  if (!ithoInternalMeasurements.empty() && systemConfig.itho_31d9 == 1)
  {
    for (const auto &internalMeasurement : ithoInternalMeasurements)
    {
      if (internalMeasurement.type == ithoDeviceMeasurements::is_int)
      {
        root[internalMeasurement.name] = internalMeasurement.value.intval;
      }
      else if (internalMeasurement.type == ithoDeviceMeasurements::is_float)
      {
        root[internalMeasurement.name] = round(internalMeasurement.value.floatval, 2);
      }
      else
      {
        root["error"] = 0;
      }
      index++;
    }
  }
  if (!ithoMeasurements.empty() && systemConfig.itho_31da == 1)
  {
    for (const auto &ithoMeasurement : ithoMeasurements)
    {
      if (ithoMeasurement.type == ithoDeviceMeasurements::is_int)
      {
        root[ithoMeasurement.name] = ithoMeasurement.value.intval;
      }
      else if (ithoMeasurement.type == ithoDeviceMeasurements::is_float)
      {
        root[ithoMeasurement.name] = round(ithoMeasurement.value.floatval, 2);
      }
      else if (ithoMeasurement.type == ithoDeviceMeasurements::is_string)
      {
        root[ithoMeasurement.name] = ithoMeasurement.value.stringval;
      }
      index++;
    }
  }
  if (!ithoStatus.empty() && systemConfig.itho_2401 == 1)
  {
    for (const auto &ithoStat : ithoStatus)
    {
      if (ithoStat.type == ithoDeviceStatus::is_byte)
      {
        root[ithoStat.name] = ithoStat.value.byteval;
      }
      else if (ithoStat.type == ithoDeviceStatus::is_int)
      {
        root[ithoStat.name] = ithoStat.value.intval;
      }
      else if (ithoStat.type == ithoDeviceStatus::is_float)
      {
        root[ithoStat.name] = round(ithoStat.value.floatval, 2);
      }
      else if (ithoStat.type == ithoDeviceStatus::is_string)
      {
        root[ithoStat.name] = ithoStat.value.stringval;
      }
      index++;
    }
  }
  if (!ithoCounters.empty() && systemConfig.itho_4210 == 1)
  {
    for (const auto &Counter : ithoCounters)
    {
      if (Counter.type == ithoDeviceMeasurements::is_int)
      {
        root[Counter.name] = Counter.value.intval;
      }
      else if (Counter.type == ithoDeviceMeasurements::is_float)
      {
        root[Counter.name] = round(Counter.value.floatval, 2);
      }
      else
      {
        root["error"] = 0;
      }
      index++;
    }
  }
  return index;
}

void getDeviceInfoJSON(JsonObject root)
{
  root["itho_devtype"] = getIthoType();
  root["itho_mfr"] = currentIthoDeviceGroup();
  root["itho_hwversion"] = currentItho_hwversion();
  root["itho_fwversion"] = currentItho_fwversion();
  root["add-on_hwid"] = WiFi.macAddress();
  root["add-on_fwversion"] = fw_version;
  if (systemConfig.fw_check)
  {
    root["add-on_fwupdate_available"] = firmwareInfo.fw_update_available == 1 ? "true" : "false";
    root["add-on_latest_fw"] = firmwareInfo.latest_fw;
    root["add-on_latest_beta_fw"] = firmwareInfo.latest_beta_fw;
  }
}

bool itho_status_ready()
{
  auto bp = [&](bool a, bool b)
  { return (a ? 1 : 0) | ((b ? 1 : 0) << 1); };

  // Build itho_init_status as 2 bits per feature:
  itho_init_status =
      bp(systemConfig.itho_31d9 == 1, !ithoInternalMeasurements.empty())  // bits 0..1
      | (bp(systemConfig.itho_31da == 1, !ithoMeasurements.empty()) << 2) // bits 2..3
      | (bp(systemConfig.itho_2401 == 1, !ithoStatus.empty()) << 4)       // bits 4..5
      | (bp(systemConfig.itho_4210 == 1, !ithoCounters.empty()) << 6);    // bits 6..7

  // Check if any feature is “activated but not filled”
  for (int i = 0; i < 4; i++)
  {
    uint8_t p = (itho_init_status >> (2 * i)) & 3; // extract 2 bits
    if ((p & 1) && !(p & 2))
      return false; // bit0=activated, bit1=filled?
  }
  return true;
}

void getRemotesInfoJSON(JsonObject root)
{

  remotes.getCapabilities(root);
}

void getIthoSettingsBackupJSON(JsonObject root)
{

  if (ithoSettingsArray != nullptr)
  {
    for (uint16_t i = 0; i < currentIthoSettingsLength(); i++)
    {
      char buf[12];
      itoa(i, buf, 10);

      if (ithoSettingsArray[i].type == ithoSettings::is_int && ithoSettingsArray[i].length == 1 && ithoSettingsArray[i].is_signed)
      {
        int8_t val;
        std::memcpy(&val, &ithoSettingsArray[i].value, sizeof(val));
        root[buf] = val;
      }
      else if (ithoSettingsArray[i].type == ithoSettings::is_int && ithoSettingsArray[i].length == 2 && ithoSettingsArray[i].is_signed)
      {
        int16_t val;
        std::memcpy(&val, &ithoSettingsArray[i].value, sizeof(val));
        root[buf] = val;
      }
      else if (ithoSettingsArray[i].type == ithoSettings::is_int && ithoSettingsArray[i].length == 4 && ithoSettingsArray[i].is_signed)
      {
        root[buf] = ithoSettingsArray[i].value;
      }
      else
      {
        uint32_t val;
        std::memcpy(&val, &ithoSettingsArray[i].value, sizeof(val));
        root[buf] = val;
      }
    }
  }
}

bool ithoExecCommand(const char *command, cmdOrigin origin)
{
  if (systemConfig.itho_vremoteapi)
  {
    D_LOG("SYS: exec i2c cmd:%s", command);
    ithoI2CCommand(0, command, origin);
  }
  else
  {
    D_LOG("SYS: exec non-i2c cmd:%s", command);
    if (strcmp(command, "away") == 0)
    {
      ithoSetSpeed(systemConfig.itho_low, origin);
    }
    else if (strcmp(command, "low") == 0)
    {
      ithoSetSpeed(systemConfig.itho_low, origin);
    }
    else if (strcmp(command, "medium") == 0)
    {
      ithoSetSpeed(systemConfig.itho_medium, origin);
    }
    else if (strcmp(command, "high") == 0)
    {
      ithoSetSpeed(systemConfig.itho_high, origin);
    }
    else if (strcmp(command, "timer1") == 0)
    {
      ithoSetTimer(systemConfig.itho_timer1, origin);
    }
    else if (strcmp(command, "timer2") == 0)
    {
      ithoSetTimer(systemConfig.itho_timer2, origin);
    }
    else if (strcmp(command, "timer3") == 0)
    {
      ithoSetTimer(systemConfig.itho_timer3, origin);
    }
    else if (strcmp(command, "cook30") == 0)
    {
      ithoSetTimer(30, origin);
    }
    else if (strcmp(command, "cook60") == 0)
    {
      ithoSetTimer(60, origin);
    }
    else if (strcmp(command, "auto") == 0)
    {
      ithoSetSpeed(systemConfig.itho_medium, origin);
    }
    else if (strcmp(command, "autonight") == 0)
    {
      ithoSetSpeed(systemConfig.itho_medium, origin);
    }
    else if (strcmp(command, "clearqueue") == 0)
    {
      clearQueue = true;
    }
    else
    {
      return false;
    }
  }

  // snprintf(modestate, sizeof(modestate), "%s", command);
  // updateMQTTmodeStatus = true;

  updateMQTTihtoStatus = true;
  return true;
}

bool ithoExecRFCommand(uint8_t remote_index, const char *command, cmdOrigin origin)
{
  bool res = true;

  D_LOG("SYS: exec rf cmd:%s, idx: %d", command, remote_index);
  disableRF_ISR();

  if (strcmp(command, "away") == 0)
  {
    rf.sendRFCommand(remote_index, IthoCommand::IthoAway);
  }
  else if (strcmp(command, "low") == 0)
  {
    rf.sendRFCommand(remote_index, IthoCommand::IthoLow);
  }
  else if (strcmp(command, "medium") == 0)
  {
    rf.sendRFCommand(remote_index, IthoCommand::IthoMedium);
  }
  else if (strcmp(command, "high") == 0)
  {
    rf.sendRFCommand(remote_index, IthoCommand::IthoHigh);
  }
  else if (strcmp(command, "timer1") == 0)
  {
    rf.sendRFCommand(remote_index, IthoCommand::IthoTimer1);
  }
  else if (strcmp(command, "timer2") == 0)
  {
    rf.sendRFCommand(remote_index, IthoCommand::IthoTimer2);
  }
  else if (strcmp(command, "timer3") == 0)
  {
    rf.sendRFCommand(remote_index, IthoCommand::IthoTimer3);
  }
  else if (strcmp(command, "cook30") == 0)
  {
    rf.sendRFCommand(remote_index, IthoCommand::IthoCook30);
  }
  else if (strcmp(command, "cook60") == 0)
  {
    rf.sendRFCommand(remote_index, IthoCommand::IthoCook60);
  }
  else if (strcmp(command, "auto") == 0)
  {
    rf.sendRFCommand(remote_index, IthoCommand::IthoAuto);
  }
  else if (strcmp(command, "autonight") == 0)
  {
    rf.sendRFCommand(remote_index, IthoCommand::IthoAutoNight);
  }
  else if (strcmp(command, "join") == 0)
  {
    rf.sendRFCommand(remote_index, IthoCommand::IthoJoin);
  }
  else if (strcmp(command, "leave") == 0)
  {
    rf.sendRFCommand(remote_index, IthoCommand::IthoLeave);
  }
  else if (strcmp(command, "motion_on") == 0)
  {
    rf.send2E10(remote_index, IthoCommand::IthoPIRmotionOn);
  }
  else if (strcmp(command, "motion_off") == 0)
  {
    rf.send2E10(remote_index, IthoCommand::IthoPIRmotionOff);
  }
  else
  {
    res = false;
  }

  char buf[32]{};
  snprintf(buf, sizeof(buf), "rfremotecmd:%s, idx:%d", (res ? command : "unknown"), remote_index);
  logLastCommand(buf, origin);

  enableRF_ISR();
  return res;
}

bool ithoSetSpeed(const char *speed, cmdOrigin origin)
{
  D_LOG("SYS: ithoSetSpeed: char speed:%s", speed);
  uint16_t val = strtoul(speed, NULL, 10);
  return ithoSetSpeed(val, origin);
}

bool ithoSetSpeed(uint16_t speed, cmdOrigin origin)
{
  if (speed < 256)
  {
    D_LOG("PWM: set speed:%d", speed);
    nextIthoVal = speed;
    nextIthoTimer = 0;
    updateItho();
  }
  else
  {
    D_LOG("PWM: set speed: value out of range");
    return false;
  }

  char buf[32]{};
  snprintf(buf, sizeof(buf), "speed:%d", speed);
  logLastCommand(buf, origin);
  return true;
}

bool ithoSetTimer(const char *timer, cmdOrigin origin)
{
  uint16_t t = strtoul(timer, NULL, 10);
  return ithoSetTimer(t, origin);
}

bool ithoSetTimer(uint16_t timer, cmdOrigin origin)
{
  D_LOG("PWM: set timer:%dmin", timer);
  if (timer > 0 && timer < 65535)
  {
    nextIthoTimer = timer;
    nextIthoVal = systemConfig.itho_high;
    updateItho();
  }
  else
  {
    return false;
  }

  char buf[32]{};
  snprintf(buf, sizeof(buf), "timer:%d", timer);
  logLastCommand(buf, origin);
  return true;
}

bool ithoSetSpeedTimer(const char *speed, const char *timer, cmdOrigin origin)
{
  uint16_t speedval = strtoul(speed, NULL, 10);
  uint16_t timerval = strtoul(timer, NULL, 10);
  return ithoSetSpeedTimer(speedval, timerval, origin);
}

bool ithoSetSpeedTimer(uint16_t speed, uint16_t timer, cmdOrigin origin)
{
  D_LOG("PWM: set speed and timer");
  if (speed < 255)
  {
    nextIthoVal = speed;
    nextIthoTimer = timer;
    updateItho();
  }
  else
  {
    return false;
  }

  char buf[32]{};
  snprintf(buf, sizeof(buf), "speed:%d,timer:%d", speed, timer);
  logLastCommand(buf, origin);
  return true;
}

void logLastCommand(const char *command, cmdOrigin origin)
{

  if (origin != REMOTE)
  {
    const char *source;
    auto it = cmdOriginMap.find(origin);
    if (it != cmdOriginMap.end())
      source = it->second;
    else
      source = cmdOriginMap.rbegin()->second;
    logLastCommand(command, source);
  }
  else
  {
    logLastCommand(command, remotes.lastRemoteName);
  }
}

void logLastCommand(const char *command, const char *source)
{

  strlcpy(lastCmd.source, source, sizeof(lastCmd.source));
  strlcpy(lastCmd.command, command, sizeof(lastCmd.command));

  if (time(nullptr))
  {
    time_t now;
    time(&now);
    lastCmd.timestamp = now;
  }
  else
  {
    lastCmd.timestamp = (time_t)millis();
  }
}

void getLastCMDinfoJSON(JsonObject root)
{

  root["source"] = lastCmd.source;
  root["command"] = lastCmd.command;
  root["timestamp"] = lastCmd.timestamp;
}

void updateItho()
{
  if (systemConfig.itho_rf_support)
  {
    IthoCMD.once_ms(150, add2queue);
  }
  else
  {
    add2queue();
  }
}

void add2queue()
{
  ithoQueue.add2queue(nextIthoVal, nextIthoTimer, systemConfig.nonQ_cmd_clearsQ);
}

void setRFdebugLevel(uint8_t level)
{
  rf.setAllowAll(true);
  if (level == 2)
  {
    rf.setAllowAll(false);
  }
  I_LOG("SYS: RF debug level = %d", level);
}

double round(float value, int precision)
{
  double pow10 = pow(10.0, precision);
  return static_cast<int>(value * pow10 + 0.5) / pow10;
}

char toHex(uint8_t c)
{
  return c < 10 ? c + '0' : c + 'A' - 10;
}

// Function to convert a hex string to an integer
int hexStringToInt(const std::string &hexStr) // throw(std::invalid_argument)
{
  int result = 0;
  for (char c : hexStr)
  {
    result *= 16;
    if (c >= '0' && c <= '9')
    {
      result += (c - '0');
    }
    else if (c >= 'A' && c <= 'F')
    {
      result += (c - 'A' + 10);
    }
    else if (c >= 'a' && c <= 'f')
    {
      result += (c - 'a' + 10);
    }
    // else
    // {
    //   throw std::invalid_argument("Invalid character in hex string: " + hexStr);
    // }
  }
  return result;
}

// Function to parse the comma-separated hex string ie "3A,D1,F1"
std::vector<int> parseHexString(const std::string &input) // throw(std::invalid_argument)
{
  std::vector<int> result;
  size_t start = 0;
  size_t end = input.find(',');

  while (end != std::string::npos)
  {
    std::string hexStr = input.substr(start, end - start);

    // Convert hex string to integer and add to the result vector
    result.push_back(hexStringToInt(hexStr));

    start = end + 1;
    end = input.find(',', start);
  }

  // Add the last hex value
  std::string hexStr = input.substr(start);
  result.push_back(hexStringToInt(hexStr));

  return result;
}

bool isNumeric(const std::string &str)
{
  return std::all_of(str.begin(), str.end(), ::isdigit);
}

// Parses a version string into numeric and optional pre-release components
// Returns false if parsing fails due to invalid format
bool parseVersion(const std::string &version, std::vector<int> &numbers, std::string &prerelease)
{
  size_t pos = 0, found;
  size_t hyphen = version.find('-');
  std::string numericPart = (hyphen != std::string::npos) ? version.substr(0, hyphen) : version;
  prerelease = (hyphen != std::string::npos) ? version.substr(hyphen + 1) : "";

  while ((found = numericPart.find('.', pos)) != std::string::npos)
  {
    std::string segment = numericPart.substr(pos, found - pos);
    if (!segment.empty() && isNumeric(segment))
    {
      numbers.push_back(stoi(segment));
    }
    else
    {
      return false; // Invalid numeric segment
    }
    pos = found + 1;
  }

  if (pos < numericPart.length())
  {
    std::string lastSegment = numericPart.substr(pos);
    if (!lastSegment.empty() && isNumeric(lastSegment))
    {
      numbers.push_back(stoi(lastSegment));
    }
    else
    {
      return false; // Invalid last segment
    }
  }

  return true; // Successfully parsed
}

int compareVersions(const std::string &v1, const std::string &v2)
{
  std::vector<int> nums1, nums2;
  std::string pre1, pre2;

  if (!parseVersion(v1, nums1, pre1) || !parseVersion(v2, nums2, pre2))
  {
    return -99; // Return error code for invalid input format
  }

  for (int i = 0; i < std::max(nums1.size(), nums2.size()); ++i)
  {
    int num1 = i < nums1.size() ? nums1[i] : 0;
    int num2 = i < nums2.size() ? nums2[i] : 0;

    if (num1 > num2)
      return 1;
    if (num1 < num2)
      return -1;
  }

  if (pre1.empty() && !pre2.empty())
    return 1; // No pre-release is higher than any pre-release
  if (!pre1.empty() && pre2.empty())
    return -1;
  if (pre1.empty() && pre2.empty())
    return 0;

  return pre1.compare(pre2); // Compare pre-releases if both are not empty
}

void check_firmware_update()
{
  WiFiClientSecure *secclient = new WiFiClientSecure;
  if (secclient)
  {
    secclient->setInsecure(); // set secure client without certificate

    HTTPClient https;

    if (https.begin(*secclient, "https://raw.githubusercontent.com/arjenhiemstra/ithowifi/master/compiled_firmware_files/firmware.json"))
    { // HTTPS
      int httpCode = https.GET();
      if (httpCode > 0)
      {
        if (httpCode == HTTP_CODE_OK && https.getSize() < MAX_FIRMWARE_HTTPS_RESPONSE_SIZE)
        {
          String payloadString = https.getString().c_str();
          const char *payload = payloadString.c_str();

          JsonDocument root;
          DeserializationError error = deserializeJson(root, payload);
          if (!error)
          {
            firmwareInfo.fw_update_available = compareVersions(root["hw_rev"][hw_revision]["latest_fw"] | "error", fw_version);

            strncpy(firmwareInfo.latest_fw, root["hw_rev"][hw_revision]["latest_fw"] | "error", sizeof(firmwareInfo.latest_fw));
            Serial.printf("latest_fw:%s\n", root["hw_rev"][hw_revision]["latest_fw"] | "error");
            strncpy(firmwareInfo.latest_beta_fw, root["hw_rev"][hw_revision]["latest_beta_fw"] | "error", sizeof(firmwareInfo.latest_beta_fw));
            Serial.printf("latest_beta_fw:%s\n", root["hw_rev"][hw_revision]["latest_beta_fw"] | "error");

            D_LOG("SYS: fw_update_available:%d", firmwareInfo.fw_update_available);
            // 1: newer version available online, 0: no new version available, -1: current version is newer than online version, -99: compare unsuccessful
          }
        }
      }
      https.end();
    }
  }
}