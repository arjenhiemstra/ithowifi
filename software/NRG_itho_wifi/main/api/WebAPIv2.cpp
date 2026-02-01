#include "api/WebAPIv2.h"
#include "api/WebAPI.h"
#include "tasks/task_web.h"

/*
This WebAPI implementation follows the JSend specification
More information can be found on github: https://github.com/omniti-labs/jsend

General information:
The WebAPI always returns a JSON which will at least contain a key "status".
The value of the status key indicates the result of the API call. This can either be "success", "fail" or "error"

In case of "success" or "fail":
- the returned JSON will always have a "data" key containing the resulting data of the request.
- the value can be a string or a JSON object or array.
- the returned JSON should contain a key "result" that contains a string with a short human readable API call result
- the returned JSON should contain a key "cmdkey" that contains a string copy of the given command when a URL encoded key/value pair is present in the API call
- the returned JSON should contain a key "cmdval" that contains a string copy of the given value when a URL encoded key/value pair is present in the API call

In case of "error":
- the returned JSON will at least contain a key "message" with a value of type string, explaining what went wrong.
- the returned JSON could also include a key "code" which contains a status code that should adhere to rfc9110

*/
void handleAPIv2(AsyncWebServerRequest *request)
{
  JsonDocument doc;
  JsonObject paramsJson = doc.to<JsonObject>();

  int params = request->params();

  for (int i = 0; i < params; i++)
  {
    const AsyncWebParameter *p = request->getParam(i);
    if (p->name().length() > 0 && !p->isFile())
    {
      paramsJson[p->name().c_str()] = p->value().c_str();
    }
  }

  AsyncWebServerAdapter adapter(request);
  ApiResponse apiresponse(&adapter, &mqttClient);
  JsonDocument response;

  time_t now;
  time(&now);
  response["timestamp"] = now;
  ApiResponse::api_response_status_t response_status = ApiResponse::status::CONTINUE;

  // if (checkAuthentication(paramsJson, response) != ApiResponse::status::CONTINUE) {
  //     apiresponse.sendFail(response);
  //     return;  // Authentication failed, exit early.
  // }

  if (systemConfig.syssec_api)
  {
    if (!request->authenticate(systemConfig.sys_username, systemConfig.sys_password))
      return request->requestAuthentication();
  }

  if (response_status == ApiResponse::status::CONTINUE)
    response_status = processGetCommands(paramsJson, response);

  if (response_status == ApiResponse::status::CONTINUE)
    response_status = processGetsettingCommands(paramsJson, response);

  if (response_status == ApiResponse::status::CONTINUE)
    response_status = processSetsettingCommands(paramsJson, response);

  if (response_status == ApiResponse::status::CONTINUE)
    response_status = processCommand(paramsJson, response);

  if (response_status == ApiResponse::status::CONTINUE)
    response_status = processSetRFremote(paramsJson, response);

  if (response_status == ApiResponse::status::CONTINUE)
    response_status = processRFremoteCommands(paramsJson, response);

  if (response_status == ApiResponse::status::CONTINUE)
    response_status = processVremoteCommands(paramsJson, response);

  if (response_status == ApiResponse::status::CONTINUE)
    response_status = processPWMSpeedTimerCommands(paramsJson, response);

  if (response_status == ApiResponse::status::CONTINUE)
    response_status = processi2csnifferCommands(paramsJson, response);

  if (response_status == ApiResponse::status::CONTINUE)
    response_status = processDebugCommands(paramsJson, response);

  if (response_status == ApiResponse::status::CONTINUE)
    response_status = processSetOutsideTemperature(paramsJson, response);

  if (response_status == ApiResponse::status::SUCCESS)
  {
    apiresponse.sendSuccess(response);
  }
  else if (response_status == ApiResponse::status::FAIL)
  {
    apiresponse.sendFail(response);
  }
  else if (!response["message"].isNull())
  {
    apiresponse.sendError(response["message"].as<const char *>());
  }
  else
  {
    apiresponse.sendError("api call could not be processed");
  }

  return;
}


ApiResponse::api_response_status_t checkAuthentication(JsonObject params, JsonDocument &response)
{
  if (!systemConfig.syssec_api)
    return ApiResponse::status::CONTINUE;

  const char *username = params["username"];
  const char *password = params["password"];

  if (username != nullptr && password != nullptr)
  {
    if (strcmp(username, systemConfig.sys_username) == 0 && strcmp(password, systemConfig.sys_password) == 0)
    {
      return ApiResponse::status::CONTINUE;
    }
  }

  response["code"] = 401;
  response["username"] = "username and/or password invalid";
  response["password"] = "password and/or username invalid";
  return ApiResponse::status::FAIL;
}

ApiResponse::api_response_status_t processGetCommands(JsonObject params, JsonDocument &response)
{
  const char *value = params["get"];

  if (value == nullptr)
    return ApiResponse::status::CONTINUE;

  response["cmdkey"] = "get";
  response["cmdvalue"] = value;

  if (strcmp(value, "currentspeed") == 0)
  {
    char ithoval[10]{};
    snprintf(ithoval, sizeof(ithoval), "%d", ithoCurrentVal);
    response["currentspeed"] = ithoval;
    return ApiResponse::status::SUCCESS;
  }
  else if (strcmp(value, "deviceinfo") == 0)
  {
    JsonObject obj = response["deviceinfo"].to<JsonObject>();

    getDeviceInfoJSON(obj);

    response.add(obj);

    return ApiResponse::status::SUCCESS;
  }
  else if (strcmp(value, "queue") == 0)
  {
    JsonArray arr = response["queue"].to<JsonArray>();
    ithoQueue.get(arr);

    response["ithoSpeed"] = ithoQueue.ithoSpeed;
    response["ithoOldSpeed"] = ithoQueue.ithoOldSpeed;
    response["fallBackSpeed"] = ithoQueue.fallBackSpeed;

    response.add(arr);

    return ApiResponse::status::SUCCESS;
  }
  else if (strcmp(value, "ithostatus") == 0)
  {
    JsonObject obj = response["ithostatus"].to<JsonObject>();
    getIthoStatusJSON(obj);

    response.add(obj);

    return ApiResponse::status::SUCCESS;
  }
  else if (strcmp(value, "lastcmd") == 0)
  {
    JsonObject obj = response["lastcmd"].to<JsonObject>();

    getLastCMDinfoJSON(obj);

    response.add(obj);

    return ApiResponse::status::SUCCESS;
  }
  else if (strcmp(value, "remotesinfo") == 0)
  {
    JsonObject obj = response["remotesinfo"].to<JsonObject>();

    getRemotesInfoJSON(obj);

    response.add(obj);

    return ApiResponse::status::SUCCESS;
  }
  else if (strcmp(value, "vremotesinfo") == 0)
  {
    JsonObject obj = response.as<JsonObject>();
    virtualRemotes.get(obj, value);
    response.add(obj);

    return ApiResponse::status::SUCCESS;
  }

  response["failreason"] = "cmdvalue not valid";
  return ApiResponse::status::FAIL;
}

ApiResponse::api_response_status_t processGetsettingCommands(JsonObject params, JsonDocument &response)
{
  const char *value = params["getsetting"];

  if (value == nullptr)
    return ApiResponse::status::CONTINUE;

  response["cmdkey"] = "getsetting";
  response["cmdval"] = value;

  const std::string str = value;

  unsigned long idx;
  try
  {
    size_t pos;
    idx = std::stoul(str.c_str(), &pos);

    if (pos != str.size())
    {
      throw std::invalid_argument("extra characters after index number");
    }
  }
  catch (const std::invalid_argument &ia)
  {
    response["code"] = 400;
    std::string err = "Invalid index argument: ";
    err += ia.what();
    response["failreason"] = err;
    return ApiResponse::status::FAIL;
  }
  catch (const std::out_of_range &oor)
  {
    response["code"] = 400;
    response["failreason"] = "Index out of range";
    return ApiResponse::status::FAIL;
  }

  if (ithoSettingsArray == nullptr)
  {
    response["code"] = 400;
    response["failreason"] = "The Itho settings array is NULL";
    return ApiResponse::status::FAIL;
  }

  if (idx >= currentIthoSettingsLength())
  {
    response["code"] = 400;
    response["failreason"] = "The setting index is invalid";
    return ApiResponse::status::FAIL;
  }

  resultPtr2410 = nullptr;
  auto timeoutmillis = millis() + 3000; // 1 sec. + 2 sec. for potential i2c queue pause on CVE devices
  // Get the settings directly instead of scheduling them, that way we can
  // present the up-to-date settings in the response.
  resultPtr2410 = sendQuery2410(idx, false);

  while (resultPtr2410 == nullptr && millis() < timeoutmillis)
  {
    // wait for result
  }

  ithoSettings *setting = &ithoSettingsArray[idx];
  double cur = 0.0;
  double min = 0.0;
  double max = 0.0;

  if (resultPtr2410 == nullptr)
  {
    response["message"] = "The I2C command failed";
    return ApiResponse::status::ERROR;
  }

  if (!decodeQuery2410(resultPtr2410, setting, &cur, &min, &max))
  {
    response["message"] = "Failed to decode the setting's value";
    return ApiResponse::status::ERROR;
  }
  else
  {
    response["label"] = getSettingLabel(idx);
    response["current"] = cur;
    response["minimum"] = min;
    response["maximum"] = max;

    return ApiResponse::status::SUCCESS;
  }

  response["code"] = 400;
  response["failreason"] = "cmdvalue not valid";
  return ApiResponse::status::FAIL;
}

ApiResponse::api_response_status_t processSetsettingCommands(JsonObject params, JsonDocument &response)
{
  const char *value = params["setsetting"];

  if (value == nullptr)
    return ApiResponse::status::CONTINUE;

  response["cmdkey"] = "setsetting";

  if (systemConfig.api_settings == 0)
  {
    response["code"] = 403;
    response["failreason"] = "The settings API is disabled";
    return ApiResponse::status::FAIL;
  }

  const char *val_param = params["value"];

  if (val_param == nullptr)
  {
    response["code"] = 400;
    response["failreason"] = "The 'value' parameter is required";
    return ApiResponse::status::FAIL;
  }

  response["newvalue"] = val_param;

  if (ithoSettingsArray == nullptr)
  {
    response["code"] = 400;
    response["failreason"] = "The Itho settings array is NULL";
    return ApiResponse::status::FAIL;
  }

  response["cmdval"] = value;

  const std::string str = value;

  unsigned long idx;
  try
  {
    size_t pos;
    idx = std::stoul(str, &pos);

    if (pos != str.size())
    {
      throw std::invalid_argument("extra characters after index number");
    }
  }
  catch (const std::invalid_argument &ia)
  {
    response["code"] = 400;
    std::string err = "Invalid index argument: ";
    err += ia.what();
    response["failreason"] = err;
    return ApiResponse::status::FAIL;
  }
  catch (const std::out_of_range &oor)
  {
    response["code"] = 400;
    response["failreason"] = "Index out of range";
    return ApiResponse::status::FAIL;
  }

  response["idx"] = idx;

  if (idx >= currentIthoSettingsLength())
  {
    response["code"] = 400;
    response["failreason"] = "The setting index is invalid";
    return ApiResponse::status::FAIL;
  }

  JsonArray arr = systemConfig.api_settings_activated.as<JsonArray>();
  bool allowed = false;
  if (arr.size() > 0)
  {
    for (JsonVariant el : arr)
    {
      if (el.as<unsigned long>() == idx)
        allowed = true;
    }
  }

  if (!allowed)
  {
    response["code"] = 400;
    response["failreason"] = "The setting index is not in the allowed list";
    return ApiResponse::status::FAIL;
  }

  ithoSettings *setting = &ithoSettingsArray[idx];

  double cur = 0.0;
  double min = 0.0;
  double max = 0.0;

  resultPtr2410 = nullptr;
  auto timeoutmillis = millis() + 3000; // 1 sec. + 2 sec. for potential i2c queue pause on CVE devices
  // Get the settings directly instead of scheduling them, that way we can
  // present the up-to-date settings in the response.
  resultPtr2410 = sendQuery2410(idx, false);

  while (resultPtr2410 == nullptr && millis() < timeoutmillis)
  {
    // wait for result
  }

  if (resultPtr2410 == nullptr)
  {
    response["message"] = "The I2C command failed";
    return ApiResponse::status::ERROR;
  }

  if (!decodeQuery2410(resultPtr2410, setting, &cur, &min, &max))
  {
    response["message"] = "Failed to decode the setting's value";
    return ApiResponse::status::ERROR;
  }

  response["minimum"] = min;
  response["maximum"] = max;

  const std::string new_val_str = val_param;

  int32_t new_val = 0;
  int32_t new_ival = 0;
  float new_dval = 0.0;

  if (setting->type == ithoSettings::is_int)
  {
    response["type"] = "int";
    try
    {
      size_t pos;
      new_ival = std::stoul(new_val_str, &pos);

      if (pos != new_val_str.size())
      {
        throw std::invalid_argument("extra characters after new setting value");
      }
    }
    catch (const std::invalid_argument &ia)
    {
      response["code"] = 400;
      std::string err = "Invalid setting value argument: ";
      err += ia.what();
      response["failreason"] = err;
      return ApiResponse::status::FAIL;
    }
    catch (const std::out_of_range &oor)
    {
      response["code"] = 400;
      response["failreason"] = "Setting value out of range";
      return ApiResponse::status::FAIL;
    }
    if (new_ival < min || new_ival > max)
    {
      response["code"] = 400;
      response["failreason"] = "The specified setting value falls outside of the allowed range";
      return ApiResponse::status::FAIL;
    }
    new_val = new_ival;
  }
  else if (setting->type == ithoSettings::is_float)
  {
    response["type"] = "float";
    try
    {
      new_dval = std::stod(new_val_str);
    }
    catch (const std::invalid_argument &ia)
    {
      response["code"] = 400;
      std::string err = "Invalid setting value argument: ";
      err += ia.what();
      response["failreason"] = err;
      return ApiResponse::status::FAIL;
    }
    catch (const std::out_of_range &oor)
    {
      response["code"] = 400;
      response["failreason"] = "Setting value out of range";
      return ApiResponse::status::FAIL;
    }

    if (new_dval < min || new_dval > max)
    {
      response["code"] = 400;
      response["failreason"] = "The specified value falls outside of the allowed range";
      return ApiResponse::status::FAIL;
    }
    float dval = new_dval * setting->divider;

    switch (ithoSettingsArray[idx].length)
    {
    case 1:
      new_val = static_cast<int32_t>(static_cast<int8_t>(dval));
      break;
    case 2:
      new_val = static_cast<int32_t>(static_cast<int16_t>(dval));
      break;
    default:
      new_val = static_cast<int32_t>(dval);
      break;
    }
  }
  else
  {
    response["failreason"] = "Setting type unknown";
    return ApiResponse::status::FAIL;
  }

  // response to not return until the settings are up to date (this is also
  // easier to work with).
  if (setSetting2410(idx, new_val, true))
  {
    response["previous"] = cur;
    return ApiResponse::status::SUCCESS;
  }
  else
  {
    response["message"] = "The I2C command failed";
    return ApiResponse::status::ERROR;
  }

  response["code"] = 400;
  response["failreason"] = "cmdvalue not valid";
  return ApiResponse::status::FAIL;
}

ApiResponse::api_response_status_t processCommand(JsonObject params, JsonDocument &response)
{
  const char *value = params["command"];

  if (value == nullptr)
    return ApiResponse::status::CONTINUE;

  response["cmdkey"] = "command";
  response["cmdvalue"] = value;
  if (systemConfig.itho_pwm2i2c != 1)
  {
    response["code"] = 400;
    response["failreason"] = "pwm2i2c protocol not enabled";
    return ApiResponse::status::FAIL;
  }
  else if (currentIthoDeviceID() != 0x4 || currentIthoDeviceID() != 0x14 || currentIthoDeviceID() != 0x1B || currentIthoDeviceID() != 0x1D) // CVE or HRU200 are the only devices that support the pwm2i2c command
  {
    response["code"] = 400;
    char failReason[100]{};
    snprintf(failReason, sizeof(failReason), "device (type: %02X) does not support pwm2i2c commands, use virtual remote instead", currentIthoDeviceID());
    response["failreason"] = failReason;
    return ApiResponse::status::FAIL;
  }
  if (systemConfig.itho_vremoteapi)
  {
    response["cmdtype"] = "i2c_cmd, idx defaulted to 0";
    ithoI2CCommand(0, value, HTMLAPI);
    response["result"] = "cmd added to i2c queue";
    return ApiResponse::status::SUCCESS;
  }
  else
  {
    response["cmdtype"] = "pwm2i2c_cmd";
    if (ithoExecCommand(value, HTMLAPI))
    {
      response["result"] = "pwm2i2c command executed";
      return ApiResponse::status::SUCCESS;
    }
    else
    {
      response["code"] = 400;
      response["failreason"] = "pwm2i2c command failed to execute";
      return ApiResponse::status::FAIL;
    }
  }

  response["code"] = 400;
  response["failreason"] = "cmdvalue not valid";
  return ApiResponse::status::FAIL;
}

/*
api.html?setrfremote=1&setting=setrfdevicedestid&settingvalue=96,C8,B6
*/
ApiResponse::api_response_status_t processSetRFremote(JsonObject params, JsonDocument &response)
{
  const char *setrfremote = params["setrfremote"];
  const char *setting = params["setting"];
  const char *settingvalue = params["settingvalue"];

  if (setrfremote == nullptr)
    return ApiResponse::status::CONTINUE;

  response["cmdkey"] = "setrfremote";
  response["settingvalue"] = settingvalue;

  uint8_t idx = 0;
  if (setrfremote != nullptr)
    idx = strtoul(setrfremote, NULL, 10);

  response["idx"] = idx;

  if (idx > remotes.getRemoteCount() - 1)
  {
    response["failreason"] = "remote index number higher than configured number of remotes";
    return ApiResponse::status::FAIL;
  }

  if (settingvalue == nullptr)
  {
    response["code"] = 400;
    response["failreason"] = "param settingvalue not present";
    return ApiResponse::status::FAIL;
  }
  if (setting == nullptr)
  {
    response["code"] = 400;
    response["failreason"] = "param setting not present";
    return ApiResponse::status::FAIL;
  }

  if (strcmp("setrfdevicebidirectional", setting) == 0)
  {
    if (strcmp(settingvalue, "true") == 0)
    {
      rfManager.radio.setRFDeviceBidirectional(idx, true);
      response["result"] = "setRFDeviceBidirectional to true";
      response["check"] = rfManager.radio.getRFDeviceBidirectional(idx);
    }
    else
    {
      rfManager.radio.setRFDeviceBidirectional(idx, false);
      response["result"] = "setRFDeviceBidirectional to false";
      response["check"] = rfManager.radio.getRFDeviceBidirectional(idx);
    }
    return ApiResponse::status::SUCCESS;
  }
  else if (strcmp("setrfdevicesourceid", setting) == 0 || strcmp("setrfdevicedestid", setting) == 0)
  {
    std::vector<int> id = parseHexString(settingvalue);

    if (id.size() != 3)
    {
      response["code"] = 400;
      response["failreason"] = "ID string should be 3 HEX decimals long formatted like: 3A,D1,FD";
      return ApiResponse::status::FAIL;
    }
    if (strcmp("setrfdevicesourceid", setting) == 0)
    {
      rfManager.radio.updateSourceID(static_cast<uint8_t>(id[0]), static_cast<uint8_t>(id[1]), static_cast<uint8_t>(id[2]), idx);
      response["result"] = "source ID updated";
      response["check"] = rfManager.radio.getSourceID(idx);
    }
    else
    {
      rfManager.radio.updateDestinationID(static_cast<uint8_t>(id[0]), static_cast<uint8_t>(id[1]), static_cast<uint8_t>(id[2]), idx);
      response["result"] = "destination ID updated";
      response["check"] = rfManager.radio.getDestinationID(idx);
    }

    return ApiResponse::status::SUCCESS;
  }

  response["code"] = 400;
  response["failreason"] = "settingvalue not valid";
  return ApiResponse::status::FAIL;
}

ApiResponse::api_response_status_t processRFremoteCommands(JsonObject params, JsonDocument &response)
{
  const char *rfremotecmd = params["rfremotecmd"];
  const char *rfremoteindex = params["rfremoteindex"];

  if (rfremotecmd == nullptr)
    return ApiResponse::status::CONTINUE;

  response["cmdkey"] = "rfremotecmd";
  response["cmdvalue"] = rfremotecmd;
  response["rfremoteindex"] = rfremoteindex;

  uint8_t idx = 0;
  if (rfremoteindex != nullptr)
    idx = strtoul(rfremoteindex, NULL, 10);

  response["idx"] = idx;

  if (idx > remotes.getRemoteCount() - 1)
  {
    response["failreason"] = "remote index number higher than configured number of remotes";
    return ApiResponse::status::FAIL;
  }

  if (ithoExecRFCommand(idx, rfremotecmd, HTMLAPI))
  {
    response["result"] = "rf command executed";
    return ApiResponse::status::SUCCESS;
  }
  else
  {
    response["failreason"] = "rf command unknown";
    return ApiResponse::status::FAIL;
  }

  response["code"] = 400;
  response["failreason"] = "cmdvalue not valid";
  return ApiResponse::status::FAIL;
}

ApiResponse::api_response_status_t processVremoteCommands(JsonObject params, JsonDocument &response)
{
  const char *vremotecmd = params["vremote"];

  if (vremotecmd == nullptr)
  {
    vremotecmd = params["vremotecmd"];
  }
  else
  {
    response["cmdkey"] = "vremote";
  }

  if (vremotecmd == nullptr)
    return ApiResponse::status::CONTINUE;
  else
    response["cmdkey"] = "vremotecmd";

  response["cmdvalue"] = vremotecmd;

  const char *vremoteindex = params["vremoteindex"];
  const char *vremotename = params["vremotename"];
  if (vremoteindex == nullptr && vremotename == nullptr)
  {
    // no index or name, valback to index 0
    response["cmdtype"] = "i2c_cmd, idx defaulted to 0";
    ithoI2CCommand(0, vremotecmd, HTMLAPI);
    response["result"] = "cmd added to i2c queue";
    return ApiResponse::status::SUCCESS;
  }
  else
  {
    // exec command for specific vremote
    int index = -1;
    if (vremoteindex != nullptr)
    {
      if (strcmp(vremoteindex, "0") == 0)
      {
        index = 0;
      }
      else
      {
        index = atoi(vremoteindex);
        if (index == 0)
          index = -1;
      }
    }
    if (vremotename != nullptr)
    {
      response["vremotename"] = vremotename;
      index = virtualRemotes.getRemoteIndexbyName(vremotename);
      if (index == -1)
      {
        response["failreason"] = "vremotename empty or unknown";
        return ApiResponse::status::FAIL;
      }
    }
    if (index > virtualRemotes.getRemoteCount() - 1)
    {
      response["failreason"] = "remote index number higher than configured number of remotes";
      return ApiResponse::status::FAIL;
    }
    if (index == -1)
    {
      response["failreason"] = "index could not parsed";
      return ApiResponse::status::FAIL;
    }
    else
    {
      response["cmdtype"] = "i2c_cmd vremote";
      response["index"] = index;

      ithoI2CCommand(index, vremotecmd, HTMLAPI);
      response["result"] = "cmd added to i2c queue";
      return ApiResponse::status::SUCCESS;
    }
  }

  response["code"] = 400;
  response["failreason"] = "cmdvalue not valid";
  return ApiResponse::status::FAIL;
}
ApiResponse::api_response_status_t processPWMSpeedTimerCommands(JsonObject params, JsonDocument &response)
{
  const char *speed = params["speed"];
  const char *timer = params["timer"];

  if (speed == nullptr && timer == nullptr)
    return ApiResponse::status::CONTINUE;

  bool cmd_result = false;
  if (speed != nullptr && timer != nullptr)
  {
    response["cmdkey"] = "speed&timer";
    response["valspeed"] = speed;
    response["valtimer"] = timer;
    response["cmdtype"] = "pwm2i2c_cmd";
    cmd_result = ithoSetSpeedTimer(speed, timer, HTMLAPI);
  }
  else if (speed != nullptr && timer == nullptr)
  {
    response["cmdkey"] = "speed";
    response["valspeed"] = speed;
    cmd_result = ithoSetSpeed(speed, HTMLAPI);
  }
  else if (timer != nullptr && speed == nullptr)
  {
    response["cmdkey"] = "timer";
    response["valtimer"] = timer;
    cmd_result = ithoSetTimer(timer, HTMLAPI);
  }
  if (cmd_result)
  {
    response["result"] = "pwm2i2c command executed";
    return ApiResponse::status::SUCCESS;
  }
  else
  {
    response["code"] = 400;
    response["failreason"] = "pwm2i2c command out of range";
    return ApiResponse::status::FAIL;
  }

  response["code"] = 400;
  response["failreason"] = "cmdvalue not valid";
  return ApiResponse::status::FAIL;
}
ApiResponse::api_response_status_t processi2csnifferCommands(JsonObject params, JsonDocument &response)
{
  const char *value = params["i2csniffer"];

  if (value == nullptr)
    return ApiResponse::status::CONTINUE;

  response["cmdkey"] = "i2csniffer";
  response["cmdvalue"] = value;

  auto *sg = static_cast<i2c_safe_guard_t *>(i2cManager.safe_guard);
  if (strcmp(value, "on") == 0)
  {
    i2cSnifferEnable();
    sg->sniffer_enabled = true;
    sg->sniffer_web_enabled = true;
    response["result"] = "i2csniffer enabled";
    return ApiResponse::status::SUCCESS;
  }
  else if (strcmp(value, "off") == 0)
  {
    i2cSnifferDisable();
    sg->sniffer_enabled = false;
    sg->sniffer_web_enabled = false;
    response["result"] = "i2csniffer disabled";
    return ApiResponse::status::SUCCESS;
  }

  response["code"] = 400;
  response["failreason"] = "cmdvalue not valid";
  return ApiResponse::status::FAIL;
}

ApiResponse::api_response_status_t processDebugCommands(JsonObject params, JsonDocument &response)
{
  const char *value = params["debug"];

  if (value == nullptr)
    return ApiResponse::status::CONTINUE;

  response["cmdkey"] = "debug";
  response["cmdvalue"] = value;

  if (strcmp(value, "level0") == 0)
  {
    setRFdebugLevel(0);
    response["result"] = "rf debug level0 set";
    return ApiResponse::status::SUCCESS;
  }
  else if (strcmp(value, "level1") == 0)
  {
    setRFdebugLevel(1);
    response["result"] = "rf debug level1 set";
    return ApiResponse::status::SUCCESS;
  }
  else if (strcmp(value, "level2") == 0)
  {
    setRFdebugLevel(2);
    response["result"] = "rf debug level2 set";
    return ApiResponse::status::SUCCESS;
  }
  else if (strcmp(value, "level3") == 0)
  {
    setRFdebugLevel(3);
    response["result"] = "rf debug level3 set";
    return ApiResponse::status::SUCCESS;
  }
  else if (strcmp(value, "reboot") == 0)
  {
    logMessagejson("Reboot requested", WEBINTERFACE);
    response["result"] = "reboot requested";
    shouldReboot = true;
    return ApiResponse::status::SUCCESS;
  }

  response["code"] = 400;
  response["failreason"] = "cmdvalue not valid";
  return ApiResponse::status::FAIL;
}

ApiResponse::api_response_status_t processSetOutsideTemperature(JsonObject params, JsonDocument &response)
{
  const char *value = params["outside_temp"];

  if (value == nullptr)
    return ApiResponse::status::CONTINUE;

  if (systemConfig.api_settings == 0)
  {
    response["code"] = 403;
    response["failreason"] = "The settings API is disabled";
    return ApiResponse::status::FAIL;
  }

  response["cmdkey"] = "outside_temp";
  response["cmdvalue"] = value;

  const std::string str = value;
  int temp;
  try
  {
    size_t pos;
    temp = std::stoi(str, &pos);

    if (pos != str.size())
    {
      throw std::invalid_argument("extra characters after the temperature");
    }
  }
  catch (const std::invalid_argument &ia)
  {
    response["code"] = 400;
    std::string err = "Invalid temperature value: ";
    err += ia.what();
    response["failreason"] = err;
    return ApiResponse::status::FAIL;
  }
  catch (const std::out_of_range &oor)
  {
    response["code"] = 400;
    response["failreason"] = "Temperature value out of range";
    return ApiResponse::status::FAIL;
  }

  // Temperature values outside of this range are likely to be wrong, so we
  // check for this just to be safe.
  if (temp <= -100 || temp >= 100)
  {
    response["code"] = 400;
    response["failreason"] = "The temperature must be between -100 and 100C";
    return ApiResponse::status::FAIL;
  }

  setSettingCE30(0, static_cast<uint16_t>(temp * 100), 0, true);
  return ApiResponse::status::SUCCESS;
}
