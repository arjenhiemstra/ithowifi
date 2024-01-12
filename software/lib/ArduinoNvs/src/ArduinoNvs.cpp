// ArduinoNvs.cpp

// Copyright (c) 2018 Sinai RnD
// Copyright (c) 2016-2017 TridentTD

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "ArduinoNvs.h"

ArduinoNvs::ArduinoNvs()
{
}

bool ArduinoNvs::begin(const char* namespaceNvs)
{
  esp_err_t err = nvs_flash_init();
  if (err != ESP_OK)
  {
    DEBUG_PRINTLN("W: NVS. Cannot init flash mem");
    if (err != ESP_ERR_NVS_NO_FREE_PAGES)
      return false;

    // erase and reinit
    DEBUG_PRINTLN("NVS. Try reinit the partition");
    const esp_partition_t *nvs_partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_NVS, NULL);
    if (nvs_partition == NULL)
      return false;
    err = esp_partition_erase_range(nvs_partition, 0, nvs_partition->size);
    esp_err_t err = nvs_flash_init();
    if (err)
      return false;
    DEBUG_PRINTLN("NVS. Partition re-formatted");
  }

  err = nvs_open(namespaceNvs, NVS_READWRITE, &_nvs_handle);
  if (err != ESP_OK)
    return false;

  return true;
}

void ArduinoNvs::close()
{
  nvs_close(_nvs_handle);
}

bool ArduinoNvs::eraseAll(bool forceCommit)
{
  esp_err_t err = nvs_erase_all(_nvs_handle);
  if (err != ESP_OK)
    return false;
  return forceCommit ? commit() : true;
}

bool ArduinoNvs::erase(const char* key, bool forceCommit)
{
  esp_err_t err = nvs_erase_key(_nvs_handle, key);
  if (err != ESP_OK)
    return false;
  return forceCommit ? commit() : true;
}

bool ArduinoNvs::commit()
{
  esp_err_t err = nvs_commit(_nvs_handle);
  if (err != ESP_OK)
    return false;
  return true;
}

bool ArduinoNvs::setInt(const char* key, uint8_t value, bool forceCommit)
{
  esp_err_t err = nvs_set_u8(_nvs_handle, key, value);
  if (err != ESP_OK)
    return false;
  return forceCommit ? commit() : true;
}

bool ArduinoNvs::setInt(const char *key, int16_t value, bool forceCommit)
{
  esp_err_t err = nvs_set_i16(_nvs_handle, key, value);
  if (err != ESP_OK)
    return false;
  return forceCommit ? commit() : true;
}

bool ArduinoNvs::setInt(const char *key, uint16_t value, bool forceCommit)
{
  esp_err_t err = nvs_set_u16(_nvs_handle, key, value);
  if (err != ESP_OK)
    return false;
  return forceCommit ? commit() : true;
}

bool ArduinoNvs::setInt(const char *key, int32_t value, bool forceCommit)
{
  esp_err_t err = nvs_set_i32(_nvs_handle, key, value);
  if (err != ESP_OK)
    return false;
  return forceCommit ? commit() : true;
}

bool ArduinoNvs::setInt(const char *key, uint32_t value, bool forceCommit)
{
  esp_err_t err = nvs_set_u32(_nvs_handle, key, value);
  if (err != ESP_OK)
    return false;
  return forceCommit ? commit() : true;
}
bool ArduinoNvs::setInt(const char *key, int64_t value, bool forceCommit)
{
  esp_err_t err = nvs_set_i64(_nvs_handle, key, value);
  if (err != ESP_OK)
    return false;
  return forceCommit ? commit() : true;
}

bool ArduinoNvs::setInt(const char *key, uint64_t value, bool forceCommit)
{
  esp_err_t err = nvs_set_u64(_nvs_handle, key, value);
  if (err != ESP_OK)
    return false;
  return forceCommit ? commit() : true;
}

bool ArduinoNvs::setString(const char *key, String value, bool forceCommit)
{
  esp_err_t err = nvs_set_str(_nvs_handle, key, value.c_str());
  if (err != ESP_OK)
    return false;
  return forceCommit ? commit() : true;
}

bool ArduinoNvs::setString(const char *key, std::string value, bool forceCommit)
{
  esp_err_t err = nvs_set_str(_nvs_handle, key, value.c_str());
  if (err != ESP_OK)
    return false;
  return forceCommit ? commit() : true;
}

bool ArduinoNvs::setString(const char *key, const char* value, bool forceCommit)
{
  esp_err_t err = nvs_set_str(_nvs_handle, key, value);
  if (err != ESP_OK)
    return false;
  return forceCommit ? commit() : true;
}

bool ArduinoNvs::setBlob(const char *key, uint8_t *blob, size_t length, bool forceCommit)
{
  DEBUG_PRINTF("ArduinoNvs::setObjct(): set obj addr = [0x%X], length = [%d]\n", (int32_t)blob, length);
  if (length == 0)
    return false;
  esp_err_t err = nvs_set_blob(_nvs_handle, key, blob, length);
  if (err)
  {
    DEBUG_PRINTF("ArduinoNvs::setObjct(): err = [0x%X]\n", err);
    return false;
  }
  return forceCommit ? commit() : true;
}

bool ArduinoNvs::setBlob(const char *key, std::vector<uint8_t> &blob, bool forceCommit)
{
  return setBlob(key, &blob[0], blob.size(), forceCommit);
}

int64_t ArduinoNvs::getInt(const char *key, int64_t default_value)
{
  uint8_t v_u8;
  int16_t v_i16;
  uint16_t v_u16;
  int32_t v_i32;
  uint32_t v_u32;
  int64_t v_i64;
  uint64_t v_u64;

  esp_err_t err;
  err = nvs_get_u8(_nvs_handle, key, &v_u8);
  if (err == ESP_OK)
    return (int64_t)v_u8;

  err = nvs_get_i16(_nvs_handle, key, &v_i16);
  if (err == ESP_OK)
    return (int64_t)v_i16;

  err = nvs_get_u16(_nvs_handle, key, &v_u16);
  if (err == ESP_OK)
    return (int64_t)v_u16;

  err = nvs_get_i32(_nvs_handle, key, &v_i32);
  if (err == ESP_OK)
    return (int64_t)v_i32;

  err = nvs_get_u32(_nvs_handle, key, &v_u32);
  if (err == ESP_OK)
    return (int64_t)v_u32;

  err = nvs_get_i64(_nvs_handle, key, &v_i64);
  if (err == ESP_OK)
    return (int64_t)v_i64;

  err = nvs_get_u64(_nvs_handle, key, &v_u64);
  if (err == ESP_OK)
    return (int64_t)v_u64;

  return default_value;
}

bool ArduinoNvs::getString(const char *key, String &res)
{
  size_t required_size;
  esp_err_t err;

  err = nvs_get_str(_nvs_handle, key, NULL, &required_size);
  if (err)
    return false;

  char value[required_size];
  err = nvs_get_str(_nvs_handle, key, value, &required_size);
  if (err)
    return false;
  res = value;
  return true;
}

bool ArduinoNvs::getString(const char *key, std::string &res)
{
  size_t required_size;
  esp_err_t err;

  err = nvs_get_str(_nvs_handle, key, NULL, &required_size);
  if (err)
    return false;

  res.resize(required_size + 1);
  err = nvs_get_str(_nvs_handle, key, &res[0], &required_size);
  if (err)
    return false;
  return true;
}

std::string ArduinoNvs::getstring(const char *key)
{
  std::string res;
  bool ok = getString(key, res);
  if (!ok)
    return std::string();
  return res;
}

String ArduinoNvs::getString(const char *key)
{
  String res;
  bool ok = getString(key, res);
  if (!ok)
    return String();
  return res;
}

size_t ArduinoNvs::getBlobSize(const char *key)
{
  size_t required_size;
  esp_err_t err = nvs_get_blob(_nvs_handle, key, NULL, &required_size);
  if (err)
  {
    if (err != ESP_ERR_NVS_NOT_FOUND) // key_not_found is not an error, just return size 0
      DEBUG_PRINTF("ArduinoNvs::getBlobSize(): err = [0x%X]\n", err);
    return 0;
  }
  return required_size;
}

bool ArduinoNvs::getBlob(const char *key, uint8_t *blob, size_t length)
{
  if (length == 0)
    return false;

  size_t required_size = getBlobSize(key);
  if (required_size == 0)
    return false;
  if (length < required_size)
    return false;

  esp_err_t err = nvs_get_blob(_nvs_handle, key, blob, &required_size);
  if (err)
  {
    DEBUG_PRINTF("ArduinoNvs::getBlob(): get object err = [0x%X]\n", err);
    return false;
  }
  return true;
}

bool ArduinoNvs::getBlob(const char *key, std::vector<uint8_t> &blob)
{
  size_t required_size = getBlobSize(key);
  if (required_size == 0)
    return false;

  blob.resize(required_size);
  esp_err_t err = nvs_get_blob(_nvs_handle, key, &blob[0], &required_size);
  if (err)
  {
    DEBUG_PRINTF("ArduinoNvs::getBlob(): get object err = [0x%X]\n", err);
    return false;
  }
  return true;
}

std::vector<uint8_t> ArduinoNvs::getBlob(const char *key)
{
  std::vector<uint8_t> res;
  bool ok = getBlob(key, res);
  if (!ok)
    res.clear();
  return res;
}

bool ArduinoNvs::setFloat(const char *key, float value, bool forceCommit)
{
  return setBlob(key, (uint8_t *)&value, sizeof(float), forceCommit);
}

float ArduinoNvs::getFloat(const char *key, float default_value)
{
  std::vector<uint8_t> res(sizeof(float));
  if (!getBlob(key, res))
    return default_value;
  return *(float *)(&res[0]);
}

ArduinoNvs NVS;
