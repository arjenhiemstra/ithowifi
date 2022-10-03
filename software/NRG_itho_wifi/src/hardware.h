#pragma once

#include "version.h"

#ifdef ESPRESSIF32_3_5_0
#define ACTIVE_FS LITTLEFS
#else
#define ACTIVE_FS LittleFS
#endif

#define STACK_SIZE_SMALL 2048
#define STACK_SIZE 4096
#define STACK_SIZE_LARGE 8192

#if defined(ESP32)
#else
#error "Usupported hardware"
#endif

// The add-on type is set in the platformio.ini file automatically, change below defines only if building in an other environment than platform.io
//#define CVE
//#define NON_CVE

#if defined(CVE)
//#warning "building vor CVE add-on"
#elif defined(NON_CVE)
//#warning "Building vor NON-CVE add-on"
#else
#error "No add-on type selected, define type in hardware.h file"
#endif

#if defined(CVE)
#define HWREVISION "2"
#define SDAPIN 21
#define SCLPIN 22
#define WIFILED 17 // 17 / 2
#define STATUSPIN 16
#define BOOTSTATE 27
#define ITHO_IRQ_PIN 4
#define FAILSAVE_PIN 14
#define ITHOSTATUS 13

#elif defined(NON_CVE)
#define HWREVISION "NON-CVE 1"
#define SDAPIN 27
#define SCLPIN 26
#define WIFILED 17 // 17 / 2
#define STATUSPIN 16
#define ITHO_IRQ_PIN 4
#define FAILSAVE_PIN 32

#else
#error "Unsupported hardware"
#endif
