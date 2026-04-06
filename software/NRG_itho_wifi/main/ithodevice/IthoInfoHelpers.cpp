#include "IthoDevice.h"
#include "devices/error_info_labels.h"

int getStatusLabelLength(const uint8_t deviceGroup, const uint8_t deviceID, const uint8_t version)
{

  const struct ihtoDeviceType *ithoDevicesptr = ithoDevices;
  const struct ihtoDeviceType *ithoDevicesendPtr = ithoDevices + ithoDevicesLength;
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

const char *getSpeedLabel()
{
  const uint8_t deviceGroup = currentIthoDeviceGroup();
  const uint8_t deviceID = currentIthoDeviceID();

  if (deviceGroup == 0x0 && deviceID == 0x4)
  {
    if (systemConfig.api_normalize == 0)
    {
      return itho31D9Labels[0].labelFull;
    }
    else
    {
      return itho31D9Labels[0].labelNormalized;
    }
  }

  return "no_speed_label_for_this_device_error";
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
uint8_t checksum(const uint8_t *buf, size_t buflen)
{
  uint8_t sum = 0;
  while (buflen--)
  {
    sum += *buf++;
  }
  return -sum;
}

size_t sendI2cQuery(uint8_t *i2c_command, size_t i2c_command_length, uint8_t *receive_buffer, i2c_cmdref_t origin)
{
  if (i2c_command == nullptr)
    return 0;
  if (receive_buffer == nullptr)
    return 0;
  if (!i2cSendBytes(i2c_command, i2c_command_length, origin))
    return 0;

  size_t len = i2cSlaveReceive(receive_buffer);
  if (!len)
    return 0;

  uint16_t opcode = (i2c_command[2] << 8 | i2c_command[3]);
  if (len > 1 && receive_buffer[len - 1] == checksum(receive_buffer, len - 1) && checkI2cReply(receive_buffer, len, opcode))
    return len;

  return 0;
}
int quickPow10(int n)
{
  static int pow10[10] = {
      1, 10, 100, 1000, 10000,
      100000, 1000000, 10000000, 100000000, 1000000000};
  if (n > (sizeof(pow10) / sizeof(pow10[0]) - 1))
    return 1;
  return pow10[n];
}

std::string i2cBufToString(const uint8_t *data, size_t len)
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

bool checkI2cReply(const uint8_t *buf, size_t buflen, const uint16_t opcode)
{
  if (buflen < 4)
    return false;

  return ((buf[2] << 8 | buf[3]) & 0x3FFF) == (opcode & 0x3FFF);
}
int castToSignedInt(int val, int length)
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

int64_t castRawBytesToInt(int32_t *valptr, int length, bool is_signed)
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
uint32_t getDividerFromDatatype(int8_t datatype)
{

  const uint32_t _divider[] =
      {
          1, 10, 100, 1000, 10000, 100000,
          1000000, 10000000, 100000000,
          1, 1, // dividers index 9 and 10 should be 0.1 and 0.01
          1, 1, 1, 256, 2};
  return _divider[datatype & 0x0f];
}

uint8_t getLengthFromDatatype(int8_t datatype)
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

bool getSignedFromDatatype(int8_t datatype)
{
  return datatype & 0x80;
}
