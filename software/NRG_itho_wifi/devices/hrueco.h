#pragma once

#include "error_info_labels.h"

const uint16_t itho_HRUecosetting2_3[] 		{ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,999};
const uint16_t itho_HRUecosetting4[] 		{ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,46,37,38,39,40,47,41,42,43,44,45,999};
const uint16_t itho_HRUecosetting6[] 		{ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,46,37,38,39,40,48,47,41,42,43,44,45,999};
const uint16_t itho_HRUecosetting7[] 		{ 0,1,2,3,4,5,6,7,8,9,10,49,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,46,37,38,39,40,48,47,41,42,43,44,45,999};
const uint16_t itho_HRUecosetting8[] 		{ 0,1,2,3,4,5,6,7,8,9,10,49,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,46,37,38,39,40,48,47,41,42,43,44,50,51,52,45,999};
const uint16_t itho_HRUecosetting10_12[] 	{ 0,1,2,3,4,5,6,7,8,9,10,49,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,27,29,31,33,35,53,54,55,56,57,58,37,38,39,40,48,47,41,42,43,44,50,51,52,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,999};

const char* ithoHRUecoSettingsLabels[] =  {
      "Percentage set 2 (%)", 
      "Temp frost protection (°C)", 
      "Frost temp offset (°K)", 
      "Min frost speed (%)", 
      "Max frost speed (%)", 
      "Frost supply fan off time (Sec)", 
      "Frost supply fan on time (Sec)", 
      "Frost speed change (%)", 
      "Frost speed change (%)", 
      "Max ratio exhaust - supply during frost protection (%)", 
      "Summer temp (°C)", 
      "Summer day time (K_hour)", 
      "Requested temp bypass regulation (°C)", 
      "Offset bypass regulation (K)", 
      "Max bypass open time (hours)", 
      "Max rpm fast var. Pwm (rpm)", 
      "Max rpm slow var. Pwm (rpm)", 
      "Max rpm  fast fixed pwm (rpm)", 
      "Speed offset (rpm)", 
      "High frequency var pwm (kHz)", 
      "Low frequency var pwm (kHz)", 
      "High frequency fixed pwm (kHz)", 
      "Low frequency fixed pwm (kHz)", 
      "Start speed vkk exhaust fan (%)", 
      "Vkk valve close time (Sec)", 
      "Rpm exhaust fan with min position potmeter low (rpm)", 
      "Rpm supply fan with min position potmeter low (rpm)", 
      "Rpm exhaust fan with max position potmeter low (rpm)", 
      "Rpm supply fan with max position potmeter low (rpm)", 
      "Rpm exhaust fan with vkk with min position potmeter low (rpm)", 
      "Rpm supply fan with vkk with min position potmeter low (rpm)", 
      "Rpm exhaust fan with vkk with max position potmeter low (rpm)", 
      "Rpm supply fan with vkk with max position potmeter low (rpm)", 
      "Rpm exhaust fan with min position potmeter high (rpm)", 
      "Rpm supply with min. Position potmeter high (rpm)", 
      "Rpm exhaust fan with max position potmeter high (rpm)", 
      "Rpm supply fan with max position potmeter high (rpm)", 
      "Stop both motors with motor fault", 
      "Correction rpm with min rpm during bypass (%)", 
      "Correction rpm with max.rpm during bypass (%)", 
      "Type of heat exchanger", 
      "Manual", 
      "Manual speed supply fan (rpm)", 
      "Manual speed exhaust fan (rpm)", 
      "Manual valve position (puls)", 
      "Not in use", 
      "Rpm supply fan with max position potmeter high (rpm)", 
      "Supply fan of during vkk test", 
      "OEM value", 
      "Summer temp GHE (°C)", 
      "Air quality", 
      "Filter live time (months)", 
      "Filter counter (hour)", 
      "Min rpm supply fan (rpm)", 
      "Max rpm supply fan (rpm)", 
      "Min value balance setting (%)", 
      "Max value balance setting (%)", 
      "Min zeta supply value", 
      "Max zeta supply value", 
      "Exhaust Constant Ca2", 
      "Exhaust Constant Ca1", 
      "Exhaust Constant Ca0", 
      "Exhaust Constant Cb2", 
      "Exhaust Constant Cb1", 
      "Exhaust Constant Ca0", 
      "Exhaust Constant Cc2", 
      "Exhaust Constant Cc1", 
      "Exhaust Constant Cc0", 
      "Supply Constant Ca2", 
      "Supply Constant Ca1", 
      "Supply Constant Ca0", 
      "Supply Constant Cb2", 
      "Supply Constant Cb1", 
      "Supply Constant Cb0", 
      "Supply Constant Cc2", 
      "Supply Constant Cc1", 
      "Supply Constant Cc0", 
      "FrostTimer", 
      "StartupCounter", 
      "BoilerTestTimer", 
      "SummerCounter", 
      "BypassTimer", 
      "MaimumOpenCounter", 
      "FilterUsagePerMin", 
      "Balance"
};

const uint8_t itho_HRUecostatus2_4[] 	{ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,255};
const uint8_t itho_HRUecostatus6_7[] 	{ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,255};
const uint8_t itho_HRUecostatus8[] 		{ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,255};
const uint8_t itho_HRUecostatus10_12[] 	{ 22,23,0,2,3,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,255};

struct ithoLabels ithoHRUecoStatusLabels[] {
	{   "Supply requested (rpm)",  	  "supply-requested_rpm" },
	{   "Supply fan speed (rpm)",  	  "supply-fan speed_rpm" },
	{   "Supply actual (rpm)",  	  "supply-actual_rpm" },
	{   "Drain requested (rpm)",  	  "drain-requested_rpm" },
	{   "Drain fan speed (rpm)",  	  "drain-fan-speed_rpm" },
	{   "Drain actual (rpm)",  		  "drain-actual_rpm" },
	{   "Temp of supply air (°C)",    "temp-of-supply-air_c" },
	{   "Temp of exhaust air (°C)",   "temp-of-exhaust-air_c" },
	{   "Status",  					  "status" },
	{   "room temp (°C)",  			  "room-temp_c" },
	{   "Outdoor temp (°C)",  		  "outdoor-temp_c" },
	{   "Valve position (pulse)",  	  "valve-position_pulse" },
	{   "Bypass position (pulse)", 	  "bypass-position_pulse" },
	{   "summer counter",  			  "summer-counter" },
	{   "Summer day",  				  "summer-day" },
	{   "Frost timer (sec)",  		  "frosttimer_sec" },
	{   "boiler timer (min)",  		  "boiler-timer_min" },
	{   "Frost lock",  				  "frost-lock" },
	{   "Current pos",  			  "current-pos" },
	{   "VKK switch",  				  "vkk-switch" },
	{   "GHE switch",  				  "ghe-switch" },
	{   "Air filter counter",  		  "air-filter counter" },
	{   "Requested speed (%)",  	  "requested-speed_perc" },
	{   "Balance (%)",  			  "balance_perc" }
};

const uint16_t *ithoHRUecoFanSettingsMap[] =   	{ nullptr, nullptr, itho_HRUecosetting2_3, itho_HRUecosetting2_3, itho_HRUecosetting4,  nullptr, itho_HRUecosetting6,  itho_HRUecosetting7,  itho_HRUecosetting8, nullptr, itho_HRUecosetting10_12, itho_HRUecosetting10_12, itho_HRUecosetting10_12 };
const uint8_t *ithoHRUecoFanStatusMap[] =   	{ nullptr, nullptr, itho_HRUecostatus2_4,  itho_HRUecostatus2_4,  itho_HRUecostatus2_4, nullptr, itho_HRUecostatus6_7, itho_HRUecostatus6_7, itho_HRUecostatus8,  nullptr, itho_HRUecostatus10_12,  itho_HRUecostatus10_12,  itho_HRUecostatus10_12  };

