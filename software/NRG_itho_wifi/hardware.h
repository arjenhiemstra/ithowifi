#pragma once

//#define CVE
#define NON_CVE

#define ACTIVE_FS LITTLEFS

#if defined (ESP32)
#else
#error "Usupported hardware"
#endif

#if defined (CVE)
#define HWREVISION "2"
#define SDAPIN       21
#define SCLPIN       22
#define WIFILED      17  // 17 / 2
#define STATUSPIN    16
#define BOOTSTATE    27
#define ITHO_IRQ_PIN 4
#define FAILSAVE_PIN 14
#define ITHOSTATUS   13

#elif defined (NON_CVE)
#define HWREVISION "NON-CVE 1"
#define SDAPIN      27
#define SCLPIN      26
#define WIFILED     17  // 17 / 2
#define STATUSPIN   16
#define ITHO_IRQ_PIN 4
#define FAILSAVE_PIN 32

#else
#error "Unsupported hardware"
#endif
