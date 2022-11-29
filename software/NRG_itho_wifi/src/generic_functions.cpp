#include "generic_functions.h"

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
    sprintf(hostName, "%s%02x%02x", espName, sys.getMac(4), sys.getMac(5));
  }
  else
  {
    strlcpy(hostName, wifiConfig.hostname, sizeof(hostName));
  }

  return hostName;
}

void getIthoStatusJSON(JsonObject root)
{
  if (SHT3x_original || SHT3x_alternative || itho_internal_hum_temp)
  {
    root["temp"] = round(ithoTemp, 1);
    root["hum"] = round(ithoHum, 1);

    auto b = 611.21 * pow(2.7183, ((18.678 - ithoTemp / 234.5) * ithoTemp) / (257.14 + ithoTemp));
    auto ppmw = b / (101325 - b) * ithoHum / 100 * 0.62145 * 1000000;
    root["ppmw"] = static_cast<int>(ppmw + 0.5);
  }

  if (!ithoInternalMeasurements.empty())
  {
    for (const auto &internalMeasurement : ithoInternalMeasurements)
    {
      if (internalMeasurement.type == ithoDeviceMeasurements::is_int)
      {
        root[internalMeasurement.name] = internalMeasurement.value.intval;
      }
      else if (internalMeasurement.type == ithoDeviceMeasurements::is_float)
      {
        root[internalMeasurement.name] = internalMeasurement.value.floatval;
      }
      else
      {
        root["error"] = 0;
      }
    }
  }
  if (!ithoMeasurements.empty())
  {
    for (const auto &ithoMeaserment : ithoMeasurements)
    {
      if (ithoMeaserment.type == ithoDeviceMeasurements::is_int)
      {
        root[ithoMeaserment.name] = ithoMeaserment.value.intval;
      }
      else if (ithoMeaserment.type == ithoDeviceMeasurements::is_float)
      {
        root[ithoMeaserment.name] = ithoMeaserment.value.floatval;
      }
      else if (ithoMeaserment.type == ithoDeviceMeasurements::is_string)
      {
        root[ithoMeaserment.name] = ithoMeaserment.value.stringval;
      }
    }
  }
  if (!ithoStatus.empty())
  {
    for (const auto &ithoStat : ithoStatus)
    {
      if (ithoStat.type == ithoDeviceStatus::is_byte)
      {
        root[ithoStat.name] = ithoStat.value.byteval;
      }
      else if (ithoStat.type == ithoDeviceStatus::is_uint)
      {
        root[ithoStat.name] = ithoStat.value.uintval;
      }
      else if (ithoStat.type == ithoDeviceStatus::is_int)
      {
        root[ithoStat.name] = ithoStat.value.intval;
      }
      else if (ithoStat.type == ithoDeviceStatus::is_float)
      {
        root[ithoStat.name] = ithoStat.value.floatval;
      }
      else if (ithoStat.type == ithoDeviceStatus::is_string)
      {
        root[ithoStat.name] = ithoStat.value.stringval;
      }
    }
  }
}

void getRemotesInfoJSON(JsonObject root)
{

  remotes.getCapabilities(root);
}

void getIthoSettingsBackupJSON(JsonObject root)
{

  if (ithoSettingsArray != nullptr)
  {
    for (int i = 0; i < currentIthoSettingsLength(); i++)
    {
      char buf[12];
      itoa(i, buf, 10);

      if (ithoSettingsArray[i].type == ithoSettings::is_int8)
      {
        int8_t val;
        std::memcpy(&val, &ithoSettingsArray[i].value, sizeof(val));
        root[buf] = val;
      }
      else if (ithoSettingsArray[i].type == ithoSettings::is_int16)
      {
        int16_t val;
        std::memcpy(&val, &ithoSettingsArray[i].value, sizeof(val));
        root[buf] = val;
      }
      else if (ithoSettingsArray[i].type == ithoSettings::is_int32)
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
  D_LOG("EXEC COMMAND:%s", command);
  if (systemConfig.itho_vremoteapi)
  {
    ithoI2CCommand(0, command, origin);
  }
  else
  {
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

  return true;
}

bool ithoSetSpeed(const char *speed, cmdOrigin origin)
{
  uint16_t val = strtoul(speed, NULL, 10);
  return ithoSetSpeed(val, origin);
}

bool ithoSetSpeed(uint16_t speed, cmdOrigin origin)
{
  if (speed < 256)
  {
    D_LOG("SET SPEED:%d", speed);
    nextIthoVal = speed;
    nextIthoTimer = 0;
    updateItho();
  }
  else
  {
    D_LOG("SET SPEED: value out of range");
    return false;
  }

  char buf[32]{};
  sprintf(buf, "speed:%d", speed);
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
  D_LOG("SET TIMER:%dmin", timer);
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
  sprintf(buf, "timer:%d", timer);
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
  D_LOG("SET SPEED AND TIMER");
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
  sprintf(buf, "speed:%d,timer:%d", speed, timer);
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
  char logBuff[LOG_BUF_SIZE]{};
  debugLevel = level;
  rf.setAllowAll(true);
  if (level == 2)
  {
    rf.setAllowAll(false);
  }
  sprintf(logBuff, "Debug level = %d", debugLevel);
  logMessagejson(logBuff, WEBINTERFACE);
  strlcpy(logBuff, "", sizeof(logBuff));
}

double round(double value, int precision)
{
  double pow10 = pow(10.0, precision);
  return static_cast<int>(value * pow10 + 0.5) / pow10;
}
