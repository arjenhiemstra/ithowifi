#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>

// Boot-phase gates (ADR-0010). All boot tasks are created up front in setup();
// each waits on the phases it depends on before its dependent init and sets its
// own bit when that init completes. Replaces the "each task starts the next"
// chain. Dependency map lives in ADR-0010.
enum BootPhase : EventBits_t
{
  PHASE_HW = 1 << 0,     // TaskInit: hardware, mutexes, sniffer
  PHASE_CONFIG = 1 << 1, // TaskConfigAndLog: filesystem + config loaded + logging
  PHASE_NET = 1 << 2,    // TaskSysControl: networkManager.initialize() + wifiInit()
  PHASE_RF = 1 << 3,     // TaskCC1101: RF module init done
  PHASE_WEB = 1 << 4,    // TaskWeb: web/websocket/mDNS up = boot complete
};

#ifndef BOOT_PHASE_TIMEOUT_MS
#define BOOT_PHASE_TIMEOUT_MS 30000
#endif

extern EventGroupHandle_t bootPhaseEvents;

void bootPhasesInit();            // create the event group; call first in setup()
void setPhase(EventBits_t bit);   // mark a phase complete
bool waitPhase(EventBits_t bits); // block (bounded) until all bits set; on timeout logs + returns false
