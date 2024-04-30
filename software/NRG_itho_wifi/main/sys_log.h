#pragma once

#define SYSLOG_QUEUE_MAX_SIZE 50
#define LOG_BUF_SIZE 128

#define EM_LOG(...) sys_log(SYSLOG_EMERG, __VA_ARGS__)
#define A_LOG(...) sys_log(SYSLOG_ALER, __VA_ARGS__)
#define C_LOG(...) sys_log(SYSLOG_CRIT, __VA_ARGS_)
#define E_LOG(...) sys_log(SYSLOG_ERR, __VA_ARGS__)
#define W_LOG(...) sys_log(SYSLOG_WARNING, __VA_ARGS__)
#define N_LOG(...) sys_log(SYSLOG_NOTICE, __VA_ARGS__)
#define I_LOG(...) sys_log(SYSLOG_INFO, __VA_ARGS__)
#define D_LOG(...) sys_log(SYSLOG_DEBUG, __VA_ARGS__)

#include <cstdio>
#include <string>
#include <queue>

#include <Arduino.h>
// #include <freertos/FreeRTOS.h>
// #include <freertos/task.h>
#include <ArduinoLog.h> // https://github.com/thijse/Arduino-Log
#include <FSFilePrint.h>
#include <FS.h>
#include <Syslog.h>
#include "WiFiUdp.h"

// #include "LittleFS.h"

// #include "config/LogConfig.h"

typedef enum
{
    SYSLOG_EMERG = 0,   /* system is unusable */
    SYSLOG_ALER = 1,    /* action must be taken immediately */
    SYSLOG_CRIT = 2,    /* critical conditions */
    SYSLOG_ERR = 3,     /* error conditions */
    SYSLOG_WARNING = 4, /* warning conditions */
    SYSLOG_NOTICE = 5,  /* normal but significant condition */
    SYSLOG_INFO = 6,    /* informational */
    SYSLOG_DEBUG = 7,   /* debug-level messages */
    ESP_SYSLOG_ERR = 13 /* ESP IDF error conditions */

} log_prio_level_t;

typedef struct
{
    log_prio_level_t code;
    std::string msg;
} log_msg;

extern WiFiUDP udpClient;
extern Syslog syslog;
extern std::deque<log_msg> syslog_queue;
extern SemaphoreHandle_t syslog_queueSemaphore;

void printTimestamp(Print *_logOutput, int logLevel);
void LogPrefixESP(Print *_logOutput, int logLevel);
void printNewline(Print *_logOutput, int logLevel);
void sys_log(log_prio_level_t log_prio, const char *inputString, ...);
int esp_vprintf(const char *fmt, va_list args);

// void sys_log(log_prio_level_t log_prio, const char *inputString, ...);
//  void sys_log(log_prio_level_t log_prio, const char *inputString, va_list args);
