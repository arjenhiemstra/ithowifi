#include "api/OpenAPI.h"
#include "config/SystemConfig.h"
#include "globals.h"
#include "webroot/openapi_json_gz.h"

// The OpenAPI spec is pre-generated at build time by build_script.py
// (build_openapi_spec()) and embedded as a gzip'd byte array
// (openapi_json_gz). Serving from flash avoids building a ~20KB
// JsonDocument on the AsyncTCP task, which used to overflow the task's
// 4KB stack and wedge the network stack on frequent Swagger UI accesses.
// The same Python generator is the single source of truth for the
// schema — if an endpoint or field needs to change, edit
// build_openapi_spec() in build_script.py and rebuild.
void handleOpenAPI(AsyncWebServerRequest *request)
{
  if (systemConfig.syssec_api)
  {
    if (!request->authenticate(systemConfig.sys_username, systemConfig.sys_password))
      return request->requestAuthentication();
  }

  AsyncWebServerResponse *response = request->beginResponse_P(
      200, "application/json", openapi_json_gz, openapi_json_gz_len);
  response->addHeader("Content-Encoding", "gzip");
  response->addHeader("Access-Control-Allow-Origin", "*");
  request->send(response);
}
