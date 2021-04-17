#ifndef __HARDWARE_H__
#define __HARDWARE_H__

#if defined (ESP8266)
  #define __HW_VERSION_ONE__
#elif defined (ESP32)
  #define __HW_VERSION_TWO__
#endif

#if defined (__HW_VERSION_ONE__) || defined (__HW_VERSION_TWO__)
#else
#error "Define hardware revision in header file hardware.h"
#endif

#if defined (__HW_VERSION_ONE__)
#define HWREVISION "1"
#define SDAPIN      4
#define SCLPIN      5
#define WIFILED     2
#define STATUSPIN   14

#elif defined (__HW_VERSION_TWO__)
#define HWREVISION "2"
#define SDAPIN      21
#define SCLPIN      22
#define WIFILED     17  // 17 / 2
#define STATUSPIN   16
#define ITHO_IRQ_PIN 4
#define FAILSAVE_PIN 14




#else
#error "Unsupported hardware"
#endif

#endif //__HARDWARE_H__
