#pragma once

#include <cstddef>
#include <cstdint>

// Fault history texts per device and firmware version (generated file).
// fwFrom: used from this firmware version up to the next entry of the same device (the lowest entry also below it).
// msgClass: 0x3222 reply [start][count][code...], 0x4205 reply [start][count][code, time (4 bytes)...]

struct ithoFaultText
{
  uint8_t code;
  uint8_t label; // index in ithoFaultLabels
};

struct ithoFaultTable
{
  uint8_t deviceGroup;
  uint8_t deviceID;
  uint8_t fwFrom;
  uint16_t msgClass;
  const ithoFaultText *texts;
  uint8_t count;
};

inline const char *const ithoFaultLabels[]{
    "No fault", //0
    "RH sensor", //1
    "CO2 sensor", //2
    "RF comm", //3
    "Valve", //4
    "Configuration", //5
    "Battery empty", //6
    "PIR not available", //7
    "PIR sensor fault", //8
    "Co2 sensor communication", //9
    "CO2 level out of range", //10
    "CO2 level doesn't change", //11
    "CO2 sensor error", //12
    "Fan stopped", //13
    "Inlet sensor fault", //14
    "Exhaust sensor fault", //15
    "Dirty filter", //16
    "No communication with motor controller", //17
    "Areaflow controller report fault", //18
    "Areaflow communication error", //19
    "PIR communication error", //20
    "No CO2 add on mounted", //21
    "PIR low battery", //22
    "CO2 sensor out of range", //23
    "CO2 sensor no change", //24
    "CO2 sensor fault", //25
    "No specific fault", //26
    "Not used (fault 2)", //27
    "Inlet sensor", //28
    "Exhaust sensor", //29
    "Not used (fault 6)", //30
    "Not used (fault 7)", //31
    "Areaflow fault", //32
    "Areaflow communication", //33
    "Communication with RF device", //34
    "No CO2 add on communication", //35
    "Low battery RF sensor", //36
    "Not used (fault 10)", //37
    "Sensor low battery", //38
    "Frostvalve fault", //39
    "Frostvalve communication error", //40
    "Filter needs to be cleaned", //41
    "Filter needs to be replaced", //42
    "Fan speed decreased because of frost", //43
    "Outside sensor fault", //44
    "NTC in vTblend defect", //45
    "Return sensor defect", //46
    "Supply sensor defect", //47
    "Fan not working correct", //48
};
inline const size_t ithoFaultLabelsLength = sizeof(ithoFaultLabels) / sizeof(ithoFaultLabels[0]);

inline const ithoFaultText itho_faults_DemandFlow_0[]{{0, 0}, {1, 1}, {2, 2}, {3, 3}, {4, 4}, {5, 5}, {6, 6}, {7, 7}, {8, 8}, {9, 9}, {10, 10}, {11, 11}, {12, 12}};
inline const ithoFaultText itho_faults_CVE_0[]{{0, 0}, {1, 13}, {2, 14}, {3, 15}, {4, 16}, {5, 17}, {6, 18}, {7, 19}, {8, 20}, {9, 21}, {10, 22}, {11, 23}, {12, 24}, {13, 25}};
inline const ithoFaultText itho_faults_CVESilent_18[]{{0, 0}, {1, 26}, {2, 27}, {3, 13}, {4, 28}, {5, 29}, {6, 30}, {7, 31}, {8, 32}, {9, 33}, {10, 34}, {11, 35}, {12, 36}, {13, 23}, {14, 24}, {15, 25}};
inline const ithoFaultText itho_faults_CVESilentExtPlus_18[]{{0, 0}, {1, 26}, {2, 27}, {3, 13}, {4, 28}, {5, 29}, {6, 30}, {7, 31}, {8, 32}, {9, 33}, {10, 37}, {11, 35}, {12, 38}, {13, 23}, {14, 24}, {15, 25}};
inline const ithoFaultText itho_faults_CVESilentExtPlus_25[]{{0, 0}, {1, 26}, {2, 27}, {3, 13}, {4, 28}, {5, 29}, {6, 30}, {7, 31}, {8, 32}, {9, 33}, {10, 34}, {11, 35}, {12, 36}, {13, 23}, {14, 24}, {15, 25}, {16, 39}, {17, 40}};
inline const ithoFaultText itho_faults_HRU250300_0[]{{0, 0}, {1, 41}, {2, 42}, {3, 43}, {51, 44}, {52, 45}, {53, 45}, {54, 46}, {55, 47}, {101, 48}};

inline const ithoFaultTable ithoFaultTables[]{
    {0x00, 0x0B, 0, 0x4205, itho_faults_DemandFlow_0, 13},
    {0x00, 0x14, 0, 0x3222, itho_faults_CVE_0, 14},
    {0x00, 0x15, 0, 0x3222, itho_faults_CVE_0, 14},
    {0x00, 0x1B, 0, 0x3222, itho_faults_CVE_0, 14},
    {0x00, 0x1B, 18, 0x3222, itho_faults_CVESilent_18, 16},
    {0x00, 0x1C, 0, 0x3222, itho_faults_CVE_0, 14},
    {0x00, 0x1D, 12, 0x3222, itho_faults_CVE_0, 14},
    {0x00, 0x1D, 18, 0x3222, itho_faults_CVESilentExtPlus_18, 16},
    {0x00, 0x1D, 21, 0x3222, itho_faults_CVESilent_18, 16},
    {0x00, 0x1D, 25, 0x3222, itho_faults_CVESilentExtPlus_25, 18},
    {0x07, 0x01, 0, 0x4205, itho_faults_HRU250300_0, 10}};
inline const size_t ithoFaultTablesLength = sizeof(ithoFaultTables) / sizeof(ithoFaultTables[0]);
