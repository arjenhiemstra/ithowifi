#pragma once

#include "error_info_labels.h"

const uint16_t itho_CVE14setting1_4[]	{ 0,1,2,3,4,5,6,7,8,9,999};
const uint16_t itho_CVE14setting5[] 	{ 0,1,2,3,4,5,6,7,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,8,9,999};
const uint16_t itho_CVE14setting6[] 	{ 0,1,2,3,4,5,6,7,19,20,32,22,23,24,25,26,27,28,29,30,31,8,9,999};

const char* ithoCVE14SettingsLabels[] =  {
     "OEM number",
     "Print version",
     "Min setting potentiometer low (rpm)",
     "Max setting potentiometer low (rpm)",
     "Min setting potentiometer high (rpm)",
     "Max setting potentiometer high (rpm)",
     "RF enable",
     "I2C mode",
     "Manual operation",
     "Speed at manual operation (rpm)",
     "Fan constant Ca2",
     "Fan constant Ca1",
     "Fan constant Ca0",
     "Fan constant Cb2",
     "Fan constant Cb1",
     "Fan constant Cb0",
     "Fan constant Cc2",
     "Fan constant Cc1",
     "Fan constant Cc0",
     "Min. Ventilation (%)",
     "CO2 concentration absent (ppm)",
     "CO2 concentration present (ppm)",
     "Min. fan setpoint at valve low (%)",
     "Min. fan setpoint at valve high (%)",
     "Min. fan setpoint at damper high and PIR (%)",
     "CO2 concentration at 100% valve low (ppm)",
     "CO2 concentration at 100% valve high (ppm)",
     "CO2 concentration at 100% valve high and PIR (ppm)",
     "Max. speed change per minute (%)",
     "Expiration time PIR presence (Min)",
     "Cycle time (min)",
     "Measuring time (min)",
     "CO2 concentration present (ppm)"
};

const uint8_t itho_CVE14status1_4[] { 0,1,2,3,4,5,6,255};
const uint8_t itho_CVE14status5_6[] { 0,1,2,3,4,5,6,7,8,9,10,11,12,13,255};

const struct ithoLabels ithoCVE14StatusLabels[] {
	{  "Ventilation level (%)",	 	"ventilation-level_perc" },
	{  "Fan setpoint (rpm)",		"fan-setpoint_rpm" },
	{  "Fan speed (rpm)", 		 	"fan-speed_rpm" },
	{  "Error", 					"error" },
	{  "Selection", 				"selection" },
	{  "Startup counter", 		 	"startup-counter" },
	{  "Total operation (hours)",  	"total-operation_hours" },
	{  "CO2 value (ppm)", 		 	"co2-value_ppm" },
	{  "CO2 ventilation (%)", 	 	"co2ventilation_perc" },
	{  "Valve", 					"valve" },
	{  "Presence timer (sec)",		"presence-timer_sec" },
	{  "Condition", 				"condition" },
	{  "Cycle timer", 			 	"cycle-timer" },
	{  "Sample timer", 			 	"sample-timer" }
};

const uint16_t *itho14SettingsMap[] =	{ nullptr, itho_CVE14setting1_4, itho_CVE14setting1_4, itho_CVE14setting1_4, itho_CVE14setting1_4, itho_CVE14setting5,  itho_CVE14setting6  };
const uint8_t  *itho14StatusMap[]   =	{ nullptr, itho_CVE14status1_4,  itho_CVE14status1_4,  itho_CVE14status1_4,  itho_CVE14status1_4,  itho_CVE14status5_6, itho_CVE14status5_6 };
