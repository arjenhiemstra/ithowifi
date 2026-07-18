#pragma once

/*
 * Device profiles for the Simulated Device Runtime.
 *
 * Each profile mirrors one entry of ithoDevices[] (ithodevice/IthoDevice.cpp) at a
 * pinned firmware version that has both a status label map and a settings map, so
 * the regular parsing code (sendQueryStatusFormat/sendQueryStatus) resolves every
 * simulated field to its real label:
 *
 *   CVE-Silent  0x00/0x1B fw 27  -> itho_CVE1Bstatus24_27   (12 fields)
 *   HRU 350     0x00/0x2B fw 4   -> itho_HRU350status1_4    (30 fields)
 *   DemandFlow  0x00/0x0B fw 12  -> itho_DemandFlowstatus4_12 (49 fields)
 *   WPU         0x00/0x0D fw 25  -> itho_WPUstatus25        (123 fields)
 *
 * statusFieldCount MUST stay equal to the label count of the pinned version
 * (sentinel-255 length of the map array); test_native_simulation pins these counts.
 *
 * Fields not listed in the override table render as defaultDatatype with value 0.
 * Datatype byte encoding (see getLengthFromDatatype/getDividerFromDatatype):
 * bit7 signed, bits6-4 length (0x10=2B, 0x20/0x70=4B, else 1B), bits3-0 divider index.
 */

#include <cstdint>

enum SimFieldRole : uint8_t
{
    SIM_ROLE_STATIC = 0,      // fixed baseline value
    SIM_ROLE_FAN_SETPOINT_PCT,
    SIM_ROLE_FAN_SETPOINT_RPM,
    SIM_ROLE_FAN_ACTUAL_PCT,
    SIM_ROLE_FAN_ACTUAL_RPM,
    SIM_ROLE_TEMP,            // shared room temperature, raw = degC * divider
    SIM_ROLE_HUMIDITY,        // shared humidity, raw = %RH * divider
    SIM_ROLE_CO2,             // shared CO2, raw = ppm
    SIM_ROLE_ERROR,           // 0, or fault code when the fault scenario is active
    SIM_ROLE_DRIFT,           // baseline + sinusoid(amplitude, period) + noise
};

struct SimField
{
    uint8_t index;     // position in the status frame
    uint8_t datatype;  // itho datatype byte
    int32_t baseline;  // raw value (already divider-scaled)
    int16_t amplitude; // raw sinusoid amplitude (SIM_ROLE_DRIFT only)
    uint16_t period_s; // sinusoid period in seconds (SIM_ROLE_DRIFT only)
    uint8_t role;
};

struct SimProfile
{
    uint8_t dg;
    uint8_t id;
    uint8_t hw;
    uint8_t fw;
    const char *name;        // matches ithoDevices[] name for the DG/ID pair
    uint8_t statusFieldCount;
    uint8_t defaultDatatype; // datatype for fields without an override
    const SimField *fields;
    uint8_t fieldCount;
    bool has31DA;
    bool has31D9;
    bool hasCounters;        // WPU 0x4210 counters
    uint8_t counterCount;
    bool da_heatExchanger;   // 31DA: supply/exhaust temps available
};

// --- CVE-Silent (0x1B) fw 27: 12 fields, datatypes from a real device capture
//     (see example replies in ithodevice/IthoStatus.cpp)
inline const SimField simFieldsCVE1B[] = {
    {0, 0x80, 40, 0, 0, SIM_ROLE_FAN_SETPOINT_PCT}, // Ventilation setpoint (%)
    {1, 0x10, 0, 0, 0, SIM_ROLE_FAN_SETPOINT_RPM},  // Fan setpoint (rpm)
    {2, 0x10, 0, 0, 0, SIM_ROLE_FAN_ACTUAL_RPM},    // Fan speed (rpm)
    {3, 0x10, 0, 0, 0, SIM_ROLE_ERROR},             // Error
    {4, 0x00, 1, 0, 0, SIM_ROLE_STATIC},            // Selection
    {5, 0x10, 42, 0, 0, SIM_ROLE_STATIC},           // Startup counter
    {6, 0x20, 8760, 0, 0, SIM_ROLE_STATIC},         // Total operation (hours)
    {7, 0x10, 0, 0, 0, SIM_ROLE_STATIC},            // Absence (min)
    {8, 0x10, 0, 0, 0, SIM_ROLE_CO2},               // Highest CO2 concentration (ppm)
    {9, 0x00, 0, 0, 0, SIM_ROLE_HUMIDITY},          // Highest RH concentration (%)
    {10, 0x92, 0, 0, 0, SIM_ROLE_HUMIDITY},         // RelativeHumidity
    {11, 0x92, 0, 0, 0, SIM_ROLE_TEMP},             // Temperature
};

// --- HRU 350 (0x2B) fw 4: 30 fields
inline const SimField simFieldsHRU350[] = {
    {0, 0x00, 40, 0, 0, SIM_ROLE_FAN_SETPOINT_PCT},  // Requested fanspeed (%)
    {1, 0x00, 100, 0, 0, SIM_ROLE_STATIC},           // Balance (%)
    {2, 0x10, 0, 0, 0, SIM_ROLE_FAN_SETPOINT_RPM},   // Supply fan (RPM)
    {3, 0x10, 0, 0, 0, SIM_ROLE_FAN_ACTUAL_RPM},     // Supply fan actual (RPM)
    {4, 0x10, 0, 0, 0, SIM_ROLE_FAN_SETPOINT_RPM},   // Exhaust fan (RPM)
    {5, 0x10, 0, 0, 0, SIM_ROLE_FAN_ACTUAL_RPM},     // Exhaust fan actual (RPM)
    {6, 0x92, 1750, 150, 3600, SIM_ROLE_DRIFT},      // Supply temp (degC)
    {7, 0x92, 2080, 120, 3600, SIM_ROLE_DRIFT},      // Exhaust temp (degC)
    {8, 0x00, 1, 0, 0, SIM_ROLE_STATIC},             // Status
    {9, 0x92, 0, 0, 0, SIM_ROLE_TEMP},               // Room temp (degC)
    {10, 0x92, 1150, 350, 5400, SIM_ROLE_DRIFT},     // Outdoor temp (degC)
    {21, 0x10, 92, 0, 0, SIM_ROLE_STATIC},           // Airfilter counter
    {22, 0x00, 0, 0, 0, SIM_ROLE_ERROR},             // Global fault code
    {23, 0x00, 1, 0, 0, SIM_ROLE_STATIC},            // Actual Mode
    {25, 0x10, 0, 0, 0, SIM_ROLE_CO2},               // Highest received CO2 value (Ppm)
    {26, 0x00, 0, 0, 0, SIM_ROLE_HUMIDITY},          // Highest received RH value (%RH)
    {27, 0x00, 92, 0, 0, SIM_ROLE_STATIC},           // Air Quality (%)
};

// --- DemandFlow (0x0B) fw 12: 49 fields
inline const SimField simFieldsDemandFlow[] = {
    {0, 0x00, 1, 0, 0, SIM_ROLE_STATIC},            // Operating status
    {1, 0x00, 2, 0, 0, SIM_ROLE_STATIC},            // Setting
    {2, 0x00, 0, 0, 0, SIM_ROLE_HUMIDITY},          // RH bathroom 1 (%)
    {3, 0x00, 42, 6, 2700, SIM_ROLE_DRIFT},         // RH bathroom 2 (%)
    {4, 0x00, 0, 0, 0, SIM_ROLE_FAN_ACTUAL_PCT},    // exhaust fan (%)
    {5, 0x10, 0, 0, 0, SIM_ROLE_CO2},               // CO2 plenum (ppm)
    {6, 0x10, 700, 120, 1800, SIM_ROLE_DRIFT},      // Calculated CO2 extractor (ppm)
    {7, 0x10, 820, 200, 1800, SIM_ROLE_DRIFT},      // Calculated CO2 kitchen (ppm)
    {9, 0x10, 760, 150, 2400, SIM_ROLE_DRIFT},      // Calculated CO2 living room 1 (ppm)
    {14, 0x10, 900, 250, 2400, SIM_ROLE_DRIFT},     // Calculated CO2 bedroom 1 (ppm)
    {42, 0x00, 0, 0, 0, SIM_ROLE_ERROR},            // Error
    {43, 0x10, 60, 0, 0, SIM_ROLE_STATIC},          // Measuring interval (sec)
};

// --- WPU heat pump (0x0D) fw 25: 123 fields
//     Override positions follow itho_WPUstatus25: label index L maps to
//     position L for L <= 16 and position L+1 for 17 <= L <= 38.
inline const SimField simFieldsWPU[] = {
    {0, 0x92, 1100, 400, 5400, SIM_ROLE_DRIFT},  // Outside temp (degC)
    {1, 0x92, 4500, 200, 3600, SIM_ROLE_DRIFT},  // Boiler temp down (degC)
    {2, 0x92, 5200, 150, 3600, SIM_ROLE_DRIFT},  // Boiler temp up (degC)
    {3, 0x92, 900, 100, 3600, SIM_ROLE_DRIFT},   // Evaporator temp (degC)
    {4, 0x92, 700, 100, 3600, SIM_ROLE_DRIFT},   // Suction gas temp (degC)
    {5, 0x92, 6500, 300, 3600, SIM_ROLE_DRIFT},  // Compressed gas temp (degC)
    {6, 0x92, 2400, 100, 3600, SIM_ROLE_DRIFT},  // Liquid temp (degC)
    {7, 0x92, 1500, 80, 3600, SIM_ROLE_DRIFT},   // Temp to source (degC)
    {8, 0x92, 1200, 80, 3600, SIM_ROLE_DRIFT},   // Temp from source (degC)
    {9, 0x92, 3500, 150, 3600, SIM_ROLE_DRIFT},  // CV supply temp (degC)
    {10, 0x92, 3000, 150, 3600, SIM_ROLE_DRIFT}, // CV return temp (degC)
    {11, 0x92, 180, 5, 7200, SIM_ROLE_DRIFT},    // CV pressure (Bar)
    {12, 0x92, 450, 50, 1800, SIM_ROLE_DRIFT},   // Compressor current (A)
    {19, 0x10, 600, 100, 1800, SIM_ROLE_DRIFT},  // Flow sensor (lt_hr)
    {21, 0x00, 40, 0, 0, SIM_ROLE_STATIC},       // Cv pump (%)
    {22, 0x00, 30, 0, 0, SIM_ROLE_STATIC},       // Well pump (%)
    {26, 0x00, 1, 0, 0, SIM_ROLE_STATIC},        // Compressor
    {29, 0x00, 0, 0, 0, SIM_ROLE_ERROR},         // Error
    {32, 0x92, 0, 0, 0, SIM_ROLE_TEMP},          // Room temp (degC)
    {33, 0x92, 2050, 0, 0, SIM_ROLE_STATIC},     // Requested room temp (degC)
    {34, 0x00, 35, 10, 3600, SIM_ROLE_DRIFT},    // Heat demand thermost. (%)
    {35, 0x00, 3, 0, 0, SIM_ROLE_STATIC},        // Status
};

inline const SimProfile simProfiles[] = {
    // dg,  id,   hw,   fw,  name,           fields, defDT, overrides,            count,                                              31DA, 31D9, cnt, cntN, daHX
    {0x00, 0x1B, 0x39, 27, "CVE-Silent", 12, 0x00, simFieldsCVE1B, sizeof(simFieldsCVE1B) / sizeof(simFieldsCVE1B[0]), true, true, false, 0, false},
    {0x00, 0x2B, 0x1B, 4, "HRU 350", 30, 0x00, simFieldsHRU350, sizeof(simFieldsHRU350) / sizeof(simFieldsHRU350[0]), true, true, false, 0, true},
    {0x00, 0x0B, 0x15, 12, "DemandFlow", 49, 0x00, simFieldsDemandFlow, sizeof(simFieldsDemandFlow) / sizeof(simFieldsDemandFlow[0]), false, true, false, 0, false},
    {0x00, 0x0D, 0x11, 25, "Heatpump", 123, 0x00, simFieldsWPU, sizeof(simFieldsWPU) / sizeof(simFieldsWPU[0]), false, false, true, 26, false},
};

inline const uint8_t simProfileCount = sizeof(simProfiles) / sizeof(simProfiles[0]);
