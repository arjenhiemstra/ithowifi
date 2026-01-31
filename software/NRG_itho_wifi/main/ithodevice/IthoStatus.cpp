#include "IthoDevice.h"
#include "devices/error_info_labels.h"
#include "devices/wpu.h"

std::vector<ithoDeviceStatus> ithoStatus;
std::vector<ithoDeviceMeasurements> ithoMeasurements;
std::vector<ithoDeviceMeasurements> ithoInternalMeasurements;
std::vector<ithoDeviceMeasurements> ithoCounters;

bool i2c_31d9_done = false;

bool itho_internal_hum_temp = false;
float ithoHum = 0;
float ithoTemp = 0;

void sendQueryDevicetype(bool updateweb)
{
  uint8_t command[] = {0x82, 0x80, 0x90, 0xE0, 0x04, 0x00, 0x8A};
  uint8_t i2cbuf[512]{};

  size_t result = sendI2cQuery(command, sizeof(command), i2cbuf, I2C_CMD_QUERY_DEVICE_TYPE);
  // example sendI2cQuery reply: 80 82 90 E0 01 25 00 01 00 1B 39 1B 01 FE FF FF FF FF FF 04 0B 07 E5 43 56 45 2D 52 46 00 00 00 00 00 00 00 00 00 00 00 00 00 00 60

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
      jsonSysmessage("ithotype", i2cBufToString(i2cbuf, result).c_str());
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

  size_t result = sendI2cQuery(&command[0], sizeof(command), &i2cbuf[0], I2C_CMD_QUERY_STATUS_FORMAT);
  // example sendI2cQuery reply: 80 82 A4 00 01 0C 80 10 10 10 00 10 20 10 10 00 92 92 29
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
      jsonSysmessage("ithostatusformat", i2cBufToString(i2cbuf, result).c_str());
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

    ithoStatus.back().is_signed = getSignedFromDatatype(i2cbuf[6 + i]);
    ithoStatus.back().length = getLengthFromDatatype(i2cbuf[6 + i]);
    ithoStatus.back().divider = getDividerFromDatatype(i2cbuf[6 + i]);

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

  size_t result = sendI2cQuery(&command[0], sizeof(command), &i2cbuf[0], I2C_CMD_QUERY_STATUS);
  // example sendI2cQuery reply: 80 82 A4 01 01 17 02 03 2C 03 2E 00 80 07 01 2A 00 00 13 2A 00 00 82 00 2C 11 6C 09 16 A6

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
      jsonSysmessage("ithostatus", i2cBufToString(i2cbuf, result).c_str());
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
          tempVal = castToSignedInt(tempVal, ithoStat.length);
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
            tempVal = castToSignedInt(tempVal, ithoStat.length);
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

// Helper function to add single byte measurement
static void addSingleByteMeasurement(const char *label, const uint8_t *i2cbuf, int offset, uint8_t errorThreshold, const std::map<uint8_t, const char *> &errorMap, bool isFloat = false, float divider = 1.0f)
{
  ithoMeasurements.push_back(ithoDeviceMeasurements());
  ithoMeasurements.back().name = label;

  if (i2cbuf[offset] > errorThreshold)
  {
    ithoMeasurements.back().type = ithoDeviceMeasurements::is_string;
    auto it = errorMap.find(i2cbuf[offset]);
    if (it != errorMap.end())
      ithoMeasurements.back().value.stringval = it->second;
    else
      ithoMeasurements.back().value.stringval = errorMap.rbegin()->second;
  }
  else if (isFloat)
  {
    ithoMeasurements.back().type = ithoDeviceMeasurements::is_float;
    ithoMeasurements.back().value.floatval = i2cbuf[offset] / divider;
  }
  else
  {
    ithoMeasurements.back().type = ithoDeviceMeasurements::is_int;
    ithoMeasurements.back().value.intval = i2cbuf[offset];
  }
}

// Helper function to add two-byte measurement
static void addTwoByteMeasurement(const char *label, const uint8_t *i2cbuf, int offset, bool checkError = false, uint8_t errorThreshold = 0x7F, const std::map<uint8_t, const char *> *errorMap = nullptr, bool isFloat = false, float divider = 1.0f)
{
  ithoMeasurements.push_back(ithoDeviceMeasurements());
  ithoMeasurements.back().name = label;

  if (checkError && i2cbuf[offset] >= errorThreshold && errorMap != nullptr)
  {
    ithoMeasurements.back().type = ithoDeviceMeasurements::is_string;
    auto it = errorMap->find(i2cbuf[offset]);
    if (it != errorMap->end())
      ithoMeasurements.back().value.stringval = it->second;
    else
      ithoMeasurements.back().value.stringval = errorMap->rbegin()->second;
  }
  else
  {
    int32_t tempVal = (i2cbuf[offset] << 8) | i2cbuf[offset + 1];
    if (isFloat)
    {
      ithoMeasurements.back().type = ithoDeviceMeasurements::is_float;
      ithoMeasurements.back().value.floatval = tempVal / divider;
    }
    else
    {
      ithoMeasurements.back().type = ithoDeviceMeasurements::is_int;
      ithoMeasurements.back().value.intval = tempVal;
    }
  }
}

void sendQuery31DA(bool updateweb)
{
  uint8_t i2cbuf[512]{};
  size_t result = sendQuery31DA(&i2cbuf[0]);
  // example sendI2cQuery reply: 80 82 B1 DA 01 1D EF 00 7F FF 2C EF 7F FF 7F FF 7F FF 7F FF F8 08 EF 98 05 00 00 00 EF EF 7F FF 7F FF 00 6F
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
      jsonSysmessage("itho31DA", i2cBufToString(i2cbuf, result).c_str());
    }
  }

  auto dataLength = i2cbuf[5];
  auto dataStart = 6;

  if (!ithoMeasurements.empty())
  {
    ithoMeasurements.clear();
  }

  // Get labels based on API normalization setting
  const int labelLen = 19;
  static const char *labels31DA[labelLen] = {0};
  for (int i = 0; i < labelLen; i++)
  {
    labels31DA[i] = systemConfig.api_normalize == 0 ?
                    itho31DALabels[i].labelFull :
                    itho31DALabels[i].labelNormalized;
  }

  // Field 0: AirQuality (%) - single byte int with error check
  if (dataLength > 0)
  {
    addSingleByteMeasurement(labels31DA[0], i2cbuf, 0 + dataStart, 200, fanSensorErrors, false);
    // Field 1: AirQ based on - single byte int, no error check
    ithoMeasurements.push_back(ithoDeviceMeasurements());
    ithoMeasurements.back().name = labels31DA[1];
    ithoMeasurements.back().type = ithoDeviceMeasurements::is_int;
    ithoMeasurements.back().value.intval = i2cbuf[1 + dataStart];
  }

  // Field 2: CO2 level (ppm) - two byte int with error check
  if (dataLength > 1)
    addTwoByteMeasurement(labels31DA[2], i2cbuf, 2 + dataStart, true, 0x7F, &fanSensorErrors2, false);

  // Field 3: Indoor humidity (%) - single byte int with error check
  if (dataLength > 3)
    addSingleByteMeasurement(labels31DA[3], i2cbuf, 4 + dataStart, 200, fanSensorErrors, false);

  // Field 4: Outdoor humidity (%) - single byte float with error check
  if (dataLength > 4)
    addSingleByteMeasurement(labels31DA[4], i2cbuf, 5 + dataStart, 200, fanSensorErrors, true, 2.0f);

  // Field 5: Exhaust temp (°C) - two byte float with error check
  if (dataLength > 5)
    addTwoByteMeasurement(labels31DA[5], i2cbuf, 6 + dataStart, true, 0x7F, &fanSensorErrors2, true, 100.0f);

  // Field 6: Supply Temp (°C) - two byte float with error check
  if (dataLength > 7)
    addTwoByteMeasurement(labels31DA[6], i2cbuf, 8 + dataStart, true, 0x7F, &fanSensorErrors2, true, 100.0f);

  // Field 7: Indoor Temp (°C) - two byte float with error check
  if (dataLength > 9)
    addTwoByteMeasurement(labels31DA[7], i2cbuf, 10 + dataStart, true, 0x7F, &fanSensorErrors2, true, 100.0f);

  // Field 8: Outdoor Temp (°C) - two byte float with error check
  if (dataLength > 11)
    addTwoByteMeasurement(labels31DA[8], i2cbuf, 12 + dataStart, true, 0x7F, &fanSensorErrors2, true, 100.0f);

  // Field 9: Speed Cap - two byte int, no error check
  if (dataLength > 13)
    addTwoByteMeasurement(labels31DA[9], i2cbuf, 14 + dataStart, false);

  // Field 10: Bypass Pos (%) - single byte float with error check
  if (dataLength > 15)
    addSingleByteMeasurement(labels31DA[10], i2cbuf, 16 + dataStart, 200, fanSensorErrors, true, 2.0f);

  // Field 11: Fan info - special case with bit masking
  if (dataLength > 16)
  {
    ithoMeasurements.push_back(ithoDeviceMeasurements());
    ithoMeasurements.back().name = labels31DA[11];
    ithoMeasurements.back().type = ithoDeviceMeasurements::is_string;
    auto it = fanInfo.find(i2cbuf[17 + dataStart] & 0x1F);
    ithoMeasurements.back().value.stringval = (it != fanInfo.end()) ? it->second : fanInfo.rbegin()->second;
  }

  // Field 12: Boiler demand (°C) - single byte float with error check
  if (dataLength > 17)
    addSingleByteMeasurement(labels31DA[12], i2cbuf, 18 + dataStart, 200, fanSensorErrors, true, 2.0f);

  // Field 13: actual power (%) - single byte float with error check
  if (dataLength > 18)
    addSingleByteMeasurement(labels31DA[13], i2cbuf, 19 + dataStart, 200, fanSensorErrors, true, 2.0f);

  // Field 14: airfilter counter - two byte int, no error check
  if (dataLength > 19)
    addTwoByteMeasurement(labels31DA[14], i2cbuf, 20 + dataStart, false);

  // Field 15: frost protection (°C) - single byte float with error check, uses fanHeatErrors
  if (dataLength > 21)
    addSingleByteMeasurement(labels31DA[15], i2cbuf, 22 + dataStart, 200, fanHeatErrors, true, 2.0f);

  // Field 16: cooling demand (°C) - single byte float with error check, uses fanHeatErrors
  if (dataLength > 22)
    addSingleByteMeasurement(labels31DA[16], i2cbuf, 23 + dataStart, 200, fanHeatErrors, true, 2.0f);

  // Field 17: boiler temp (°C) - two byte float with error check
  if (dataLength > 23)
    addTwoByteMeasurement(labels31DA[17], i2cbuf, 24 + dataStart, true, 0x7F, &fanSensorErrors2, true, 100.0f);

  // Field 18: Preheater power actual (%) - two byte float with error check
  if (dataLength > 25)
    addTwoByteMeasurement(labels31DA[18], i2cbuf, 26 + dataStart, true, 0x7F, &fanSensorErrors2, true, 100.0f);
}
size_t sendQuery31DA(uint8_t *receive_buffer)
{
  uint8_t command[] = {0x82, 0x80, 0x31, 0xDA, 0x04, 0x00, 0xEF};

  return sendI2cQuery(&command[0], sizeof(command), &receive_buffer[0], I2C_CMD_QUERY_31DA);
}

void sendQuery31D9(bool updateweb)
{
  i2c_31d9_done = false;

  uint8_t i2cbuf[512]{};
  size_t result = sendQuery31D9(&i2cbuf[0]);
  // example sendI2cQuery reply: 80 82 B1 D9 01 10 86 04 0A 20 20 20 20 20 20 20 20 20 20 20 20 00 4F

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
      jsonSysmessage("itho31D9", i2cBufToString(i2cbuf, result).c_str());
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

  return sendI2cQuery(&command[0], sizeof(command), &receive_buffer[0], I2C_CMD_QUERY_31D9);
}

void sendQueryCounters(bool updateweb)
{
  uint8_t command[] = {0x82, 0x80, 0x42, 0x10, 0x04, 0x00, 0xA8};
  uint8_t i2cbuf[512]{};

  size_t result = sendI2cQuery(&command[0], sizeof(command), &i2cbuf[0], I2C_CMD_QUERY_STATUS);

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
      jsonSysmessage("ithocounters", i2cBufToString(i2cbuf, result).c_str());
    }
  }

  // i2c reply structure:
  //  [I2C addr ][I2C command   ][len payload ][N values][2byte value] ... [2byte value][chk ]
  //  0x80,0x82,  0xC2,0x10,0x01, 0x35,         0x1A,     0x0D,0x26,   ...  0x0A,0xF8,   0xFF

  int valPos = 7; // position of first 2byte value
  int Nvalues = i2cbuf[6];
  if (Nvalues > ithoWPUCounterLabelLength)
  {
    E_LOG("SYS: error - WPU Counter array too long. Counters not read.");
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
