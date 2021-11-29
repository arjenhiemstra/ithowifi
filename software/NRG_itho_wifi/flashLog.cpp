#include "flashLog.h"
#include "hardware.h"

FSFilePrint filePrint(LITTLEFS, "/logfile", 2, 10000);
SemaphoreHandle_t mutexLogTask;

void printTimestamp(Print * _logOutput) {
#if defined (HW_VERSION_ONE)
  if (time(nullptr)) {
    time_t now;
    struct tm * timeinfo;
    time(&now);
    timeinfo = localtime(&now);

    char timeStringBuff[50];  // 50 chars should be enough
    strftime(timeStringBuff, sizeof(timeStringBuff), "%F %T ", timeinfo);
    _logOutput->print(timeStringBuff);
  } else
#elif defined (HW_VERSION_TWO)
  struct tm timeinfo;
  if (getLocalTime(&timeinfo, 0)) {
    char timeStringBuff[50];  // 50 chars should be enough
    strftime(timeStringBuff, sizeof(timeStringBuff), "%F %T ", &timeinfo);
    _logOutput->print(timeStringBuff);
  } else
#endif
  {
    char c[32];
    sprintf(c, "%10lu ", millis());
    _logOutput->print(c);
  }
}

void printNewline(Print * _logOutput) {
  _logOutput->print("\n");
}

void logInput(const char * inputString) {
#if defined (HW_VERSION_TWO)
  yield();
  if (xSemaphoreTake(mutexLogTask, (TickType_t) 500 / portTICK_PERIOD_MS) == pdTRUE) {
#endif

    filePrint.open();

    Log.notice(inputString);

    filePrint.close();

#if defined (ENABLE_SERIAL)
    D_LOG("%s\n", inputString);
#endif

#if defined (HW_VERSION_TWO)
    xSemaphoreGive(mutexLogTask);
  }
#endif


}
