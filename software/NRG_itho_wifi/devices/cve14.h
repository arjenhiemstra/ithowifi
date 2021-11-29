#pragma once

#include "error_info_labels.h"

const uint16_t itho_CVE14setting1_4[]	{ 0,1,2,3,4,5,6,7,8,9,999};
const uint16_t itho_CVE14setting5[] 	{ 0,1,2,3,4,5,6,7,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,8,9,999};
const uint16_t itho_CVE14setting6[] 	{ 0,1,2,3,4,5,6,7,19,20,32,22,23,24,25,26,27,28,29,30,31,8,9,999};

const __FlashStringHelper *ithoCVE14SettingsLabels[] =  {
    F("OEM number"),
    F("Print version"),
    F("Min setting potentiometer low (rpm)"),
    F("Max setting potentiometer low (rpm)"),
    F("Min setting potentiometer high (rpm)"),
    F("Max setting potentiometer high (rpm)"),
    F("RF enable"),
    F("I2C mode"),
    F("Manual operation"),
    F("Speed at manual operation (rpm)"),
    F("Fan constant Ca2"),
    F("Fan constant Ca1"),
    F("Fan constant Ca0"),
    F("Fan constant Cb2"),
    F("Fan constant Cb1"),
    F("Fan constant Cb0"),
    F("Fan constant Cc2"),
    F("Fan constant Cc1"),
    F("Fan constant Cc0"),
    F("Min. Ventilation (%)"),
    F("CO2 concentration absent (ppm)"),
    F("CO2 concentration present (ppm)"),
    F("Min. fan setpoint at valve low (%)"),
    F("Min. fan setpoint at valve high (%)"),
    F("Min. fan setpoint at damper high and PIR (%)"),
    F("CO2 concentration at 100% valve low (ppm)"),
    F("CO2 concentration at 100% valve high (ppm)"),
    F("CO2 concentration at 100% valve high and PIR (ppm)"),
    F("Max. speed change per minute (%)"),
    F("Expiration time PIR presence (Min)"),
    F("Cycle time (min)"),
    F("Measuring time (min)"),
    F("CO2 concentration present (ppm)")
};

const uint8_t itho_CVE14status1_4[] { 0,1,2,3,4,5,6,255};
const uint8_t itho_CVE14status5_6[] { 0,1,2,3,4,5,6,7,8,9,10,11,12,13,255};

struct ithoLabels ithoCVE14StatusLabels[] {
	{ F("Ventilation level (%)"),	F("ventilation-level_perc") },
	{ F("Fan setpoint (rpm)"),		F("fan-setpoint_rpm") },
	{ F("Fan speed (rpm)"), 		F("fan-speed_rpm") },
	{ F("Error"), 				F("error") },
	{ F("Selection"), 				F("selection") },
	{ F("Startup counter"), 		F("startup-counter") },
	{ F("Total operation (hours)"), F("total-operation_hours") },
	{ F("CO2 value (ppm)"), 		F("co2-value_ppm") },
	{ F("CO2 ventilation (%)"), 	F("co2ventilation_perc") },
	{ F("Valve"), 					F("valve") },
	{ F("Presence timer (sec)"),	F("presence-timer_sec") },
	{ F("Condition"), 				F("condition") },
	{ F("Cycle timer"), 			F("cycle-timer") },
	{ F("Sample timer"), 			F("sample-timer") }
};

const uint16_t *itho14SettingsMap[] =	{ nullptr, itho_CVE14setting1_4, itho_CVE14setting1_4, itho_CVE14setting1_4, itho_CVE14setting1_4, itho_CVE14setting5,  itho_CVE14setting6  };
const uint8_t  *itho14StatusMap[]   =	{ nullptr, itho_CVE14status1_4,  itho_CVE14status1_4,  itho_CVE14status1_4,  itho_CVE14status1_4,  itho_CVE14status5_6, itho_CVE14status5_6 };
