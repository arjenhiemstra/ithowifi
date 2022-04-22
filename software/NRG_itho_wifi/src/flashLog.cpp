#include "flashLog.h"

FSFilePrint filePrint(ACTIVE_FS, "/logfile", 2, 10000);
SemaphoreHandle_t mutexLogTask;

void printTimestamp(Print *_logOutput, int logLevel)
{
  struct tm timeinfo;
  if (getLocalTime(&timeinfo, 0))
  {
    char timeStringBuff[50]; // 50 chars should be enough
    strftime(timeStringBuff, sizeof(timeStringBuff), "%F %T ", &timeinfo);
    _logOutput->print(timeStringBuff);
  }
  else
  {
    char c[32];
    sprintf(c, "%10lu ", millis());
    _logOutput->print(c);
  }
}

void printNewline(Print *_logOutput, int logLevel)
{
  _logOutput->print("\n");
}

void logInput(const char *inputString)
{
  yield();
  if (xSemaphoreTake(mutexLogTask, (TickType_t)500 / portTICK_PERIOD_MS) == pdTRUE)
  {

    filePrint.open();

    Log.notice(inputString);

    filePrint.close();

#if defined(ENABLE_SERIAL)
    D_LOG("%s\n", inputString);
#endif

    xSemaphoreGive(mutexLogTask);
  }
}
