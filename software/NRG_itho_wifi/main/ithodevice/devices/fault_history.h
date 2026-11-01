#pragma once

#include <cstdint>

// Fault history texts per device and firmware version (generated file).
// fwFrom: used from this firmware version up to the next entry of the same device (the lowest entry also below it).
// msgClass: 0x3222 reply [start][count][code...], 0x4205 reply [start][count][code, time (4 bytes)...]

struct ithoFaultText
{
  uint8_t code;
  const char *text;
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

inline const ithoFaultText itho_faults_DemandFlow_0[]{
    {0, "No fault"},
    {1, "RH sensor"},
    {2, "CO2 sensor"},
    {3, "RF comm"},
    {4, "Valve"},
    {5, "Configuration"},
    {6, "Battery empty"},
    {7, "PIR not available"},
    {8, "PIR sensor fault"},
    {9, "Co2 sensor communication"},
    {10, "CO2 level out of range"},
    {11, "CO2 level doesn't change"},
    {12, "CO2 sensor error"}};
inline const ithoFaultText itho_faults_CVE_0[]{
    {0, "No fault"},
    {1, "Fan stopped"},
    {2, "Inlet sensor fault"},
    {3, "Exhaust sensor fault"},
    {4, "Dirty filter"},
    {5, "No communication with motor controller"},
    {6, "Areaflow controller report fault"},
    {7, "Areaflow communication error"},
    {8, "PIR communication error"},
    {9, "No CO2 add on mounted"},
    {10, "PIR low battery"},
    {11, "CO2 sensor out of range"},
    {12, "CO2 sensor no change"},
    {13, "CO2 sensor fault"}};
inline const ithoFaultText itho_faults_Extended_0[]{
    {0, "No fault"},
    {1, "Fan stopped"},
    {2, "Inlet sensor fault"},
    {3, "Exhaust sensor fault"},
    {4, "Dirty filter"},
    {5, "No communication with motor controller"},
    {6, "Areaflow controller report fault"},
    {7, "Areaflow communication error"},
    {8, "PIR communication error"},
    {9, "No CO2 add on mounted"},
    {10, "PIR low battery"},
    {11, "CO2 sensor out of range"},
    {12, "CO2 sensor no change"},
    {13, "CO2 sensor fault"}};
inline const ithoFaultText itho_faults_CVESilent_0[]{
    {0, "No fault"},
    {1, "Fan stopped"},
    {2, "Inlet sensor fault"},
    {3, "Exhaust sensor fault"},
    {4, "Dirty filter"},
    {5, "No communication with motor controller"},
    {6, "Areaflow controller report fault"},
    {7, "Areaflow communication error"},
    {8, "PIR communication error"},
    {9, "No CO2 add on mounted"},
    {10, "PIR low battery"},
    {11, "CO2 sensor out of range"},
    {12, "CO2 sensor no change"},
    {13, "CO2 sensor fault"}};
inline const ithoFaultText itho_faults_CVESilent_18[]{
    {0, "No fault"},
    {1, "No specific fault"},
    {2, "Not used (fault 2)"},
    {3, "Fan stopped"},
    {4, "Inlet sensor"},
    {5, "Exhaust sensor"},
    {6, "Not used (fault 6)"},
    {7, "Not used (fault 7)"},
    {8, "Areaflow fault"},
    {9, "Areaflow communication"},
    {10, "Communication with RF device"},
    {11, "No CO2 add on communication"},
    {12, "Low battery RF sensor"},
    {13, "CO2 sensor out of range"},
    {14, "CO2 sensor no change"},
    {15, "CO2 sensor fault"}};
inline const ithoFaultText itho_faults_CVESilentExt_0[]{
    {0, "No fault"},
    {1, "Fan stopped"},
    {2, "Inlet sensor fault"},
    {3, "Exhaust sensor fault"},
    {4, "Dirty filter"},
    {5, "No communication with motor controller"},
    {6, "Areaflow controller report fault"},
    {7, "Areaflow communication error"},
    {8, "PIR communication error"},
    {9, "No CO2 add on mounted"},
    {10, "PIR low battery"},
    {11, "CO2 sensor out of range"},
    {12, "CO2 sensor no change"},
    {13, "CO2 sensor fault"}};
inline const ithoFaultText itho_faults_CVESilentExtPlus_12[]{
    {0, "No fault"},
    {1, "Fan stopped"},
    {2, "Inlet sensor fault"},
    {3, "Exhaust sensor fault"},
    {4, "Dirty filter"},
    {5, "No communication with motor controller"},
    {6, "Areaflow controller report fault"},
    {7, "Areaflow communication error"},
    {8, "PIR communication error"},
    {9, "No CO2 add on mounted"},
    {10, "PIR low battery"},
    {11, "CO2 sensor out of range"},
    {12, "CO2 sensor no change"},
    {13, "CO2 sensor fault"}};
inline const ithoFaultText itho_faults_CVESilentExtPlus_18[]{
    {0, "No fault"},
    {1, "No specific fault"},
    {2, "Not used (fault 2)"},
    {3, "Fan stopped"},
    {4, "Inlet sensor"},
    {5, "Exhaust sensor"},
    {6, "Not used (fault 6)"},
    {7, "Not used (fault 7)"},
    {8, "Areaflow fault"},
    {9, "Areaflow communication"},
    {10, "Not used (fault 10)"},
    {11, "No CO2 add on communication"},
    {12, "Sensor low battery"},
    {13, "CO2 sensor out of range"},
    {14, "CO2 sensor no change"},
    {15, "CO2 sensor fault"}};
inline const ithoFaultText itho_faults_CVESilentExtPlus_21[]{
    {0, "No fault"},
    {1, "No specific fault"},
    {2, "Not used (fault 2)"},
    {3, "Fan stopped"},
    {4, "Inlet sensor"},
    {5, "Exhaust sensor"},
    {6, "Not used (fault 6)"},
    {7, "Not used (fault 7)"},
    {8, "Areaflow fault"},
    {9, "Areaflow communication"},
    {10, "Communication with RF device"},
    {11, "No CO2 add on communication"},
    {12, "Low battery RF sensor"},
    {13, "CO2 sensor out of range"},
    {14, "CO2 sensor no change"},
    {15, "CO2 sensor fault"}};
inline const ithoFaultText itho_faults_CVESilentExtPlus_25[]{
    {0, "No fault"},
    {1, "No specific fault"},
    {2, "Not used (fault 2)"},
    {3, "Fan stopped"},
    {4, "Inlet sensor"},
    {5, "Exhaust sensor"},
    {6, "Not used (fault 6)"},
    {7, "Not used (fault 7)"},
    {8, "Areaflow fault"},
    {9, "Areaflow communication"},
    {10, "Communication with RF device"},
    {11, "No CO2 add on communication"},
    {12, "Low battery RF sensor"},
    {13, "CO2 sensor out of range"},
    {14, "CO2 sensor no change"},
    {15, "CO2 sensor fault"},
    {16, "Frostvalve fault"},
    {17, "Frostvalve communication error"}};
inline const ithoFaultText itho_faults_HRU250300_0[]{
    {0, "No fault"},
    {1, "Filter needs to be cleaned"},
    {2, "Filter needs to be replaced"},
    {3, "Fan speed decreased because of frost"},
    {51, "Outside sensor fault"},
    {52, "NTC in vTblend defect"},
    {53, "NTC in vTblend defect"},
    {54, "Return sensor defect"},
    {55, "Supply sensor defect"},
    {101, "Fan not working correct"}};

inline const ithoFaultTable ithoFaultTables[]{
    {0x00, 0x0B, 0, 0x4205, itho_faults_DemandFlow_0, 13},
    {0x00, 0x14, 0, 0x3222, itho_faults_CVE_0, 14},
    {0x00, 0x15, 0, 0x3222, itho_faults_Extended_0, 14},
    {0x00, 0x1B, 0, 0x3222, itho_faults_CVESilent_0, 14},
    {0x00, 0x1B, 18, 0x3222, itho_faults_CVESilent_18, 16},
    {0x00, 0x1C, 0, 0x3222, itho_faults_CVESilentExt_0, 14},
    {0x00, 0x1D, 12, 0x3222, itho_faults_CVESilentExtPlus_12, 14},
    {0x00, 0x1D, 18, 0x3222, itho_faults_CVESilentExtPlus_18, 16},
    {0x00, 0x1D, 21, 0x3222, itho_faults_CVESilentExtPlus_21, 16},
    {0x00, 0x1D, 25, 0x3222, itho_faults_CVESilentExtPlus_25, 18},
    {0x07, 0x01, 0, 0x4205, itho_faults_HRU250300_0, 10}};
inline const size_t ithoFaultTablesLength = sizeof(ithoFaultTables) / sizeof(ithoFaultTables[0]);
