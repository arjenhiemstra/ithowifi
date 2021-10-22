#pragma once

#define CVE
//#define NON_CVE

#if defined (ESP8266)
  #define HW_VERSION_ONE
#elif defined (ESP32)
  #define HW_VERSION_TWO
#endif

#if defined (HW_VERSION_ONE) || defined (HW_VERSION_TWO)
#else
#error "Define hardware revision in header file hardware.h"
#endif


#if defined (HW_VERSION_ONE) && defined (CVE)
#define HWREVISION "1"
#define SDAPIN      4
#define SCLPIN      5
#define WIFILED     2
#define STATUSPIN   14

#elif defined (HW_VERSION_TWO) && defined (CVE)
#define HWREVISION "2"
#define SDAPIN       21
#define SCLPIN       22
#define WIFILED      17  // 17 / 2
#define STATUSPIN    16
#define BOOTSTATE    27
#define ITHO_IRQ_PIN 4
#define FAILSAVE_PIN 14
#define ITHOSTATUS   13

#elif defined (HW_VERSION_TWO) && defined (NON_CVE)
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
