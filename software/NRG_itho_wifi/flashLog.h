#pragma once

#include <stdio.h>
#include <string>
#include <cstring>
#include <Arduino.h>
#include <Ticker.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <ArduinoLog.h>       // https://github.com/thijse/Arduino-Log [1.0.3]
#include <SpiffsFilePrint.h>  // https://github.com/PRosenb/SPIFFS_FilePrint [1.0.0]

extern SpiffsFilePrint filePrint;

extern SemaphoreHandle_t mutexLogTask;

void printTimestamp(Print * _logOutput);
void printNewline(Print * _logOutput);
void logInput(const char * inputString);
