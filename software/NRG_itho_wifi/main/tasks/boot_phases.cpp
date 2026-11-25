#include "tasks/boot_phases.h"
#include "esp_log.h"

EventGroupHandle_t bootPhaseEvents = nullptr;

void bootPhasesInit()
{
  bootPhaseEvents = xEventGroupCreate();
}

void setPhase(EventBits_t bit)
{
  if (bootPhaseEvents != nullptr)
    xEventGroupSetBits(bootPhaseEvents, bit);
}

bool waitPhase(EventBits_t bits)
{
  if (bootPhaseEvents == nullptr)
    return false;
  const EventBits_t got = xEventGroupWaitBits(bootPhaseEvents, bits, pdFALSE, pdTRUE,
                                              pdMS_TO_TICKS(BOOT_PHASE_TIMEOUT_MS));
  if ((got & bits) != bits)
  {
    // Low-level log (not sys_log): a phase can time out before the syslog queue
    // exists, so this path must not depend on it.
    ESP_LOGW("BOOT", "timeout waiting for phase(s) 0x%02X (have 0x%02X)",
             static_cast<unsigned>(bits), static_cast<unsigned>(got));
    return false;
  }
  return true;
}
