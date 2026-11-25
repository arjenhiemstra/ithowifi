#pragma once

// One field of the 2401 status reply and how a poll updates it. Free of Arduino and
// FreeRTOS so the exact firmware code runs in the native tests (test_native_status_field).

#include <cstdint>
#include <cstring>

struct ithoDeviceStatus
{
  const char *name;
  enum : uint8_t
  {
    is_byte,
    is_int,
    is_float
  } type;
  uint8_t length;
  union
  {
    uint8_t byteval;
    int32_t intval;
    float floatval;
  } value;
  uint32_t divider;
  uint8_t updated;
  bool is_signed;
  const char *error; // set while the Itho reports this field's sensor as unavailable
  ithoDeviceStatus() : name(nullptr), updated(0), error(nullptr) {};
};

inline int castToSignedInt(int val, int length)
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

inline bool ithoStatusIsHumidity(const char *name)
{
  return strcmp("RelativeHumidity", name) == 0 || strcmp("relativehumidity", name) == 0;
}

inline bool ithoStatusIsTemperature(const char *name)
{
  return strcmp("Temperature", name) == 0 || strcmp("temperature", name) == 0;
}

// Whether this poll's value is the Itho's way of saying the field's sensor gave no reading.
inline bool ithoStatusSensorUnavailable(const ithoDeviceStatus &ithoStat)
{
  if (strcmp("Highest CO2 concentration (ppm)", ithoStat.name) == 0 || strcmp("highest-co2-concentration_ppm", ithoStat.name) == 0)
    return ithoStat.value.intval == 0x8200;
  if (strcmp("Highest RH concentration (%)", ithoStat.name) == 0 || strcmp("highest-rh-concentration_perc", ithoStat.name) == 0)
    return ithoStat.value.intval == 0xEF;
  if (ithoStatusIsHumidity(ithoStat.name) || ithoStatusIsTemperature(ithoStat.name))
    return !(ithoStat.value.floatval > 1.0 && ithoStat.value.floatval < 100.0);
  return false;
}

// Store one poll's raw value in the field. sensorError is the text a field shows when the
// Itho reports its sensor as unavailable.
inline void ithoStatusUpdateField(ithoDeviceStatus &ithoStat, int tempVal, const char *sensorError)
{
  if (ithoStat.type == ithoDeviceStatus::is_byte)
  {
    if (ithoStat.value.byteval == (uint8_t)tempVal)
    {
      ithoStat.updated = 0;
    }
    else
    {
      ithoStat.updated = 1;
      ithoStat.value.byteval = (uint8_t)tempVal;
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

  ithoStat.error = ithoStatusSensorUnavailable(ithoStat) ? sensorError : nullptr;
}
