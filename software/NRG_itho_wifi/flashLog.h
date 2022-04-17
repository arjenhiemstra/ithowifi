#pragma once

#include <cstdio>
#include <string>

#include <Arduino.h>
#include <Ticker.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <ArduinoLog.h>       // https://github.com/thijse/Arduino-Log [1.0.3]
#include <FSFilePrint.h>      
#include <FS.h>
#include <LITTLEFS.h>

#include "hardware.h"

//#include <SpiffsFilePrint.h>  // https://github.com/PRosenb/SPIFFS_FilePrint [1.0.0]

#if defined (ENABLE_CMDLOG)
extern Logging LogCommand;
extern FSFilePrint cmdLog;
extern bool cmdLogInitialised;
#endif

extern FSFilePrint filePrint;


extern SemaphoreHandle_t mutexLogTask;

void printTimestamp(Print *_logOutput, int logLevel);
void printNewline(Print *_logOutput, int logLevel);
void logInput(const char * inputString);
#if defined (ENABLE_CMDLOG)
void logCmd(const char * inputString);
#endif
