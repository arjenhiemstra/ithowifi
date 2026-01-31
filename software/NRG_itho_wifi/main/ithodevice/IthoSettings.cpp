#include "IthoDevice.h"
#include "devices/error_info_labels.h"

ithoSettings *ithoSettingsArray = nullptr;
int32_t *resultPtr2410 = nullptr;
bool i2c_result_updateweb = false;

struct retryItem
{
  int index;
  int retries;
};

std::vector<retryItem> retryList;

int getSettingsLength(const uint8_t deviceGroup, const uint8_t deviceID, const uint8_t version)
{

  const struct ihtoDeviceType *ithoDevicesptr = ithoDevices;
  const struct ihtoDeviceType *ithoDevicesendPtr = ithoDevices + ithoDevicesLength;
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
    i2cQueueAddCmd([index]()
                      { resultPtr2410 = sendQuery2410(index, false); });
    i2cQueueAddCmd([index, loop]()
                      { processSettingResult(index, loop); });
  }
  else // -> update setting values from other source (ie. debug page)
  {
    // index2410 = i;
    // i2c_result_updateweb = updateweb;
    resultPtr2410 = nullptr;
    // get2410 = true;
    i2cQueueAddCmd([index, updateweb]()
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

void updateSetting(const uint8_t index, const int32_t value, bool webupdate)
{
  // i2c_result_updateweb = webupdate;
  // index2410 = index;
  // value2410 = value;
  // set2410 = true;

  if (ithoSettingsArray != nullptr && ithoSettingsArray[index].length == 0)
  { // if settings loaded from browser cache, first retrieve the current value of the setting
    i2cQueueAddCmd([index]()
                      { getSetting(index, true, false, false); });
    while (true) // wait for the setting to be retrieved or timeout after 2 sec
    {
      unsigned long timeout_ms = 2000; // 2 seconds
      unsigned long start_time = millis();

      if (millis() - start_time > timeout_ms)
      {
        break;
      }
      if (ithoSettingsArray[index].length != 0)
      {
        break;
      }
      vTaskDelay(pdMS_TO_TICKS(100)); // Small delay to avoid hogging CPU
    }
  }

  i2cQueueAddCmd([index, value, webupdate]()
                    { setSetting2410(index, value, webupdate); });

  i2cQueueAddCmd([index]()
                    { getSetting(index, true, false, false); });
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

  if (!i2cSendBytes(command, sizeof(command), I2C_CMD_SET_CE30))
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

  D_LOG("I2C: Sending 4030: %s", i2cBufToString(command, sizeof(command)).c_str());
  if (updateweb)
    jsonSysmessage("itho_4030_result", i2cBufToString(command, sizeof(command)).c_str());

  if (!i2cSendBytes(command, sizeof(command), I2C_CMD_SET_CE30))
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

  if (!i2cSendBytes(command, sizeof(command), I2C_CMD_QUERY_2410))
  {
    if (updateweb)
    {
      jsonSysmessage("itho2410", "failed");
    }
    return nullptr;
  }

  uint8_t i2cbuf[512] = {0};
  size_t len = i2cSlaveReceive(i2cbuf);
  if (len > 1 && i2cbuf[len - 1] == checksum(i2cbuf, len - 1) && checkI2cReply(i2cbuf, len, 0x2410))
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

    ithoSettingsArray[index].is_signed = getSignedFromDatatype(i2cbuf[22]);
    ithoSettingsArray[index].length = getLengthFromDatatype(i2cbuf[22]);
    ithoSettingsArray[index].divider = getDividerFromDatatype(i2cbuf[22]);
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
      jsonSysmessage("itho2410", i2cBufToString(i2cbuf, len).c_str());

      char tempbuffer0[256] = {0};
      char tempbuffer1[256] = {0};
      char tempbuffer2[256] = {0};

      int64_t val0, val1, val2;
      val0 = castRawBytesToInt(&values[0], ithoSettingsArray[index].length, ithoSettingsArray[index].is_signed);
      val1 = castRawBytesToInt(&values[1], ithoSettingsArray[index].length, ithoSettingsArray[index].is_signed);
      val2 = castRawBytesToInt(&values[2], ithoSettingsArray[index].length, ithoSettingsArray[index].is_signed);

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
    D_LOG("I2C: itho2410: failed for index:%d", index);
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
  int64_t a = castRawBytesToInt(ptr + 0, len, is_signed);
  int64_t b = castRawBytesToInt(ptr + 1, len, is_signed);
  int64_t c = castRawBytesToInt(ptr + 2, len, is_signed);

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

  jsonSysmessage("itho2410set", i2cBufToString(command, sizeof(command)).c_str());

  if (!i2cSendBytes(command, sizeof(command), I2C_CMD_SET_2410))
  {
    if (updateweb)
    {
      jsonSysmessage("itho2410setres", "failed");
    }
    return false;
  }

  uint8_t i2cbuf[512] = {0};
  size_t len = i2cSlaveReceive(i2cbuf);
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
