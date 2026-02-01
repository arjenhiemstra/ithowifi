#include "api/WebAPI.h"
#include "api/WebAPIv1.h"
#include "api/WebAPIv2.h"
#include "tasks/task_web.h"

void handleAPI(AsyncWebServerRequest *request)
{
  if (systemConfig.api_version == 1)
    handleAPIv1(request);
  else if (systemConfig.api_version == 2)
    handleAPIv2(request);
}
