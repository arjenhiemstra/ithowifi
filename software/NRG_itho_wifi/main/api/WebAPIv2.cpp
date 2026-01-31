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
