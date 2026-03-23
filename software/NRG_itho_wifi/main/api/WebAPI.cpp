#include "api/WebAPI.h"
#include "api/WebAPIv1.h"
#include "api/WebAPIv2.h"
#include "tasks/task_web.h"

void handleAPI(AsyncWebServerRequest *request)
{
  if (systemConfig.api_version == 0)
    handleAPIv1(request);
  else
    handleAPIv2(request);
}
