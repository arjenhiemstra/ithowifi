#pragma once

#include "error_info_labels.h"

const uint16_t itho_HRUecosetting2_3[] 		{ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,999};
const uint16_t itho_HRUecosetting4[] 		{ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,46,37,38,39,40,47,41,42,43,44,45,999};
const uint16_t itho_HRUecosetting6[] 		{ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,46,37,38,39,40,48,47,41,42,43,44,45,999};
const uint16_t itho_HRUecosetting7[] 		{ 0,1,2,3,4,5,6,7,8,9,10,49,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,46,37,38,39,40,48,47,41,42,43,44,45,999};
const uint16_t itho_HRUecosetting8[] 		{ 0,1,2,3,4,5,6,7,8,9,10,49,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,46,37,38,39,40,48,47,41,42,43,44,50,51,52,45,999};
const uint16_t itho_HRUecosetting10_12[] 	{ 0,1,2,3,4,5,6,7,8,9,10,49,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,27,29,31,33,35,53,54,55,56,57,58,37,38,39,40,48,47,41,42,43,44,50,51,52,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,999};

const __FlashStringHelper *ithoHRUecoSettingsLabels[] =  {
    F("Percentage set 2 (%)"),
    F("Temp frost protection (°C)"),
    F("Frost temp offset (°K)"),
    F("Min frost speed (%)"),
    F("Max frost speed (%)"),
    F("Frost supply fan off time (Sec)"),
    F("Frost supply fan on time (Sec)"),
    F("Frost speed change (%)"),
    F("Frost speed change (%)"),
    F("Max ratio exhaust - supply during frost protection (%)"),
    F("Summer temp (°C)"),
    F("Summer day time (K_hour)"),
    F("Requested temp bypass regulation (°C)"),
    F("Offset bypass regulation (K)"),
    F("Max bypass open time (hours)"),
    F("Max rpm fast var. Pwm (rpm)"),
    F("Max rpm slow var. Pwm (rpm)"),
    F("Max rpm  fast fixed pwm (rpm)"),
    F("Speed offset (rpm)"),
    F("High frequency var pwm (kHz)"),
    F("Low frequency var pwm (kHz)"),
    F("High frequency fixed pwm (kHz)"),
    F("Low frequency fixed pwm (kHz)"),
    F("Start speed vkk exhaust fan (%)"),
    F("Vkk valve close time (Sec)"),
    F("Rpm exhaust fan with min position potmeter low (rpm)"),
    F("Rpm supply fan with min position potmeter low (rpm)"),
    F("Rpm exhaust fan with max position potmeter low (rpm)"),
    F("Rpm supply fan with max position potmeter low (rpm)"),
    F("Rpm exhaust fan with vkk with min position potmeter low (rpm)"),
    F("Rpm supply fan with vkk with min position potmeter low (rpm)"),
    F("Rpm exhaust fan with vkk with max position potmeter low (rpm)"),
    F("Rpm supply fan with vkk with max position potmeter low (rpm)"),
    F("Rpm exhaust fan with min position potmeter high (rpm)"),
    F("Rpm supply with min. Position potmeter high (rpm)"),
    F("Rpm exhaust fan with max position potmeter high (rpm)"),
    F("Rpm supply fan with max position potmeter high (rpm)"),
    F("Stop both motors with motor fault"),
    F("Correction rpm with min rpm during bypass (%)"),
    F("Correction rpm with max.rpm during bypass (%)"),
    F("Type of heat exchanger"),
    F("Manual"),
    F("Manual speed supply fan (rpm)"),
    F("Manual speed exhaust fan (rpm)"),
    F("Manual valve position (puls)"),
    F("Not in use"),
    F("Rpm supply fan with max position potmeter high (rpm)"),
    F("Supply fan of during vkk test"),
    F("OEM value"),
    F("Summer temp GHE (°C)"),
    F("Air quality"),
    F("Filter live time (months)"),
    F("Filter counter (hour)"),
    F("Min rpm supply fan (rpm)"),
    F("Max rpm supply fan (rpm)"),
    F("Min value balance setting (%)"),
    F("Max value balance setting (%)"),
    F("Min zeta supply value"),
    F("Max zeta supply value"),
    F("Exhaust Constant Ca2"),
    F("Exhaust Constant Ca1"),
    F("Exhaust Constant Ca0"),
    F("Exhaust Constant Cb2"),
    F("Exhaust Constant Cb1"),
    F("Exhaust Constant Ca0"),
    F("Exhaust Constant Cc2"),
    F("Exhaust Constant Cc1"),
    F("Exhaust Constant Cc0"),
    F("Supply Constant Ca2"),
    F("Supply Constant Ca1"),
    F("Supply Constant Ca0"),
    F("Supply Constant Cb2"),
    F("Supply Constant Cb1"),
    F("Supply Constant Cb0"),
    F("Supply Constant Cc2"),
    F("Supply Constant Cc1"),
    F("Supply Constant Cc0"),
    F("FrostTimer"),
    F("StartupCounter"),
    F("BoilerTestTimer"),
    F("SummerCounter"),
    F("BypassTimer"),
    F("MaimumOpenCounter"),
    F("FilterUsagePerMin"),
    F("Balance")
};

const uint8_t itho_HRUecostatus2_4[] 	{ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,255};
const uint8_t itho_HRUecostatus6_7[] 	{ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,255};
const uint8_t itho_HRUecostatus8[] 		{ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,255};
const uint8_t itho_HRUecostatus10_12[] 	{ 22,23,0,2,3,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,255};

struct ithoLabels ithoHRUecoStatusLabels[] {
	{ F("Supply requested (rpm)"), 	F("supply-requested_rpm") },
	{ F("Supply fan speed (rpm)"), 	F("supply-fan speed_rpm") },
	{ F("Supply actual (rpm)"), 	F("supply-actual_rpm") },
	{ F("Drain requested (rpm)"), 	F("drain-requested_rpm") },
	{ F("Drain fan speed (rpm)"), 	F("drain-fan-speed_rpm") },
	{ F("Drain actual (rpm)"), 		F("drain-actual_rpm") },
	{ F("Temp of supply air (°C)"), F("temp-of-supply-air_c") },
	{ F("Temp of exhaust air (°C)"),F("temp-of-exhaust-air_c") },
	{ F("Status"), 					F("status") },
	{ F("room temp (°C)"), 			F("room-temp_c") },
	{ F("Outdoor temp (°C)"), 		F("outdoor-temp_c") },
	{ F("Valve position (pulse)"), 	F("valve-position_pulse") },
	{ F("Bypass position (pulse)"),	F("bypass-position_pulse") },
	{ F("summer counter"), 			F("summer-counter") },
	{ F("Summer day"), 				F("summer-day") },
	{ F("Frost timer (sec)"), 		F("frosttimer_sec") },
	{ F("boiler timer (min)"), 		F("boiler-timer_min") },
	{ F("Frost lock"), 				F("frost-lock") },
	{ F("Current pos"), 			F("current-pos") },
	{ F("VKK switch"), 				F("vkk-switch") },
	{ F("GHE switch"), 				F("ghe-switch") },
	{ F("Air filter counter"), 		F("air-filter counter") },
	{ F("Requested speed (%)"), 	F("requested-speed_perc") },
	{ F("Balance (%)"), 			F("balance_perc") }
};

const uint16_t *ithoHRUecoFanSettingsMap[] =   	{ nullptr, nullptr, itho_HRUecosetting2_3, itho_HRUecosetting2_3, itho_HRUecosetting4,  nullptr, itho_HRUecosetting6,  itho_HRUecosetting7,  itho_HRUecosetting8, nullptr, itho_HRUecosetting10_12, itho_HRUecosetting10_12, itho_HRUecosetting10_12 };
const uint8_t *ithoHRUecoFanStatusMap[] =   	{ nullptr, nullptr, itho_HRUecostatus2_4,  itho_HRUecostatus2_4,  itho_HRUecostatus2_4, nullptr, itho_HRUecostatus6_7, itho_HRUecostatus6_7, itho_HRUecostatus8,  nullptr, itho_HRUecostatus10_12,  itho_HRUecostatus10_12,  itho_HRUecostatus10_12  };

