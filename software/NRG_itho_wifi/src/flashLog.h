#pragma once

#include <cstdio>
#include <string>

#include <Arduino.h>
#include <Ticker.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <ArduinoLog.h> // https://github.com/thijse/Arduino-Log [1.0.3]
#include <FSFilePrint.h>
#include <FS.h>

#ifdef ESPRESSIF32_3_5_0
#include <LITTLEFS.h>
#else
#include <LittleFS.h>
#endif

#include "hardware.h"
#include "Dbglog.h"

//#include <SpiffsFilePrint.h>  // https://github.com/PRosenb/SPIFFS_FilePrint [1.0.0]

extern FSFilePrint filePrint;

extern SemaphoreHandle_t mutexLogTask;

void printTimestamp(Print *_logOutput, int logLevel);
void printNewline(Print *_logOutput, int logLevel);
void logInput(const char *inputString);
