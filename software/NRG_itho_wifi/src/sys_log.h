#pragma once

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

#include <Arduino.h>
#include <Ticker.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <ArduinoLog.h> // https://github.com/thijse/Arduino-Log [1.0.3]
#include <FSFilePrint.h>
#include <FS.h>
#include <Syslog.h>
#include <WifiUdp.h>

#ifdef ESPRESSIF32_3_5_0
#include <LITTLEFS.h>
#else
#include <LittleFS.h>
#endif

//#include "LogConfig.h"

typedef enum
{
    SYSLOG_EMERG = 0,   /* system is unusable */
    SYSLOG_ALER = 1,    /* action must be taken immediately */
    SYSLOG_CRIT = 2,    /* critical conditions */
    SYSLOG_ERR = 3,     /* error conditions */
    SYSLOG_WARNING = 4, /* warning conditions */
    SYSLOG_NOTICE = 5,  /* normal but significant condition */
    SYSLOG_INFO = 6,    /* informational */
    SYSLOG_DEBUG = 7    /* debug-level messages */
} log_prio_level_t;

typedef struct
{
    log_prio_level_t code;
    char msg[LOG_BUF_SIZE]{};
} log_msg;





extern WiFiUDP udpClient;
extern Syslog syslog;
extern QueueHandle_t syslog_queue;

void printTimestamp(Print *_logOutput, int logLevel);
void printNewline(Print *_logOutput, int logLevel);
void sys_log(log_prio_level_t log_prio, const char *inputString, ...);

//void sys_log(log_prio_level_t log_prio, const char *inputString, ...);
// void sys_log(log_prio_level_t log_prio, const char *inputString, va_list args);
