#include "api/WebAPI.h"
#include "api/WebAPIv1.h"
#include "tasks/task_web.h"

void handleAPI(AsyncWebServerRequest *request)
{
  handleAPIv1(request);
}
