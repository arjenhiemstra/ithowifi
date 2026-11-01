#pragma once

#include "error_info_labels.h"

inline const uint16_t itho_AWHeatpumpsetting1[]{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 999};
inline const uint16_t itho_AWHeatpumpsetting2[]{0, 1, 2, 3, 4, 5, 6, 15, 8, 9, 10, 11, 12, 13, 7, 16, 17, 999};

inline const char *ithoAWHeatpumpSettingsLabels[] = {
    "Electricity price (cent)",
    "Gas price (cent)",
    "Fan setting",
    "Heatpump overruntime (min)",
    "Hybrid setpoint mix state (°C)",
    "Minimum cv temperature LT (°C)",
    "Maximum cv temperature LT (°C)",
    "Mix valve open",
    "Heatpump standby",
    "Heatpump test",
    "Mix ventilation air",
    "CV kick in delay time (°Cmin)",
    "Dry heating",
    "Pump overruntime (min)",
    "User reset",
    "Mixing valve start position",
    "Pump setting",
    "Fan night reduction"};

inline const uint8_t itho_AWHeatpumpstatus1[]{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 255};
inline const uint8_t itho_AWHeatpumpstatus2[]{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 34, 29, 30, 31, 35, 36, 255};

inline const struct ithoLabels ithoAWHeatpumpStatusLabels[]{
    {"HP state", "hp-state"},
    {"Error message number", "error-message-number"},
    {"Warning number", "warning-number"},
    {"Measured air mix temperature (°C)", "measured-air-mix-temperature_c"},
    {"WCV valve position (steps)", "wcv-valve-position_steps"},
    {"Discharge temperature (°C)", "discharge-temperature_c"},
    {"Water side heat exchanger temperature (°C)", "water-side-heat-exchanger-temperature_c"},
    {"Air side heat exchanger temperature (°C)", "air-side-heat-exchanger-temperature_c"},
    {"EXV expansion valve position (steps)", "exv-expansion-valve-position_steps"},
    {"Suction temperature (°C)", "suction-temperature_c"},
    {"Number of compressor starts", "number-of-compressor-starts"},
    {"HP up time (Days)", "hp-up-time_days"},
    {"Last blocking events", "last-blocking-events"},
    {"Last locking events", "last-locking-events"},
    {"Fan speed (rpm)", "fan-speed_rpm"},
    {"HP return temperature (°C)", "hp-return-temperature_c"},
    {"HP mix temperature (°C)", "hp-mix-temperature_c"},
    {"HP supply temperature (°C)", "hp-supply-temperature_c"},
    {"Compressor speed (Hz)", "compressor-speed_hz"},
    {"HMI HR clock", "hmi-hr-clock"},
    {"CH Boiler control for mixing state", "ch-boiler-control-for-mixing-state"},
    {"Pump speed", "pump-speed"},
    {"Pump overrun timer (Min)", "pump-overrun-timer_min"},
    {"Hardware version CC print", "hardware-version-cc-print"},
    {"Firmware checksum CC print", "firmware-checksum-cc-print"},
    {"Calculated air mix temperature (°C)", "calculated-air-mix-temperature_c"},
    {"Super heating (°C)", "super-heating_c"},
    {"Hardware version HR", "hardware-version-hr"},
    {"Firmware version HR", "firmware-version-hr"},
    {"HMI hardware version", "hmi-hardware-version"},
    {"HMI software version", "hmi-software-version"},
    {"HP mode", "hp-mode"},
    {"Request OT zone 1", "request-ot-zone-1"},
    {"Request on-off zone 2", "request-on-off-zone-2"},
    {"Error code HR", "error-code-hr"},
    {"Thermostat zone 1", "thermostat-zone-1"},
    {"Thermostat zone 2", "thermostat-zone-2"}};

inline const uint16_t *ithoAWHeatpumpSettingsMap[] = {nullptr, itho_AWHeatpumpsetting1, itho_AWHeatpumpsetting2};
inline const uint8_t *ithoAWHeatpumpStatusMap[] = {nullptr, itho_AWHeatpumpstatus1, itho_AWHeatpumpstatus2};
