#pragma once

const uint8_t itho_HRUecosetting2_3[] { 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,255};
const uint8_t itho_HRUecosetting4_6[] { 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,44,37,38,39,45,40,41,42,43,255};
const uint8_t itho_HRUecosetting7[] { 0,1,2,3,4,5,6,7,8,9,10,46,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,44,37,38,39,45,40,41,42,43,255};
const uint8_t itho_HRUecosetting8[] { 0,1,2,3,4,5,6,7,8,9,10,46,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,44,37,38,39,45,40,41,42,43,47,48,49,255};
const uint8_t itho_HRUecosetting10_12[] { 0,1,2,3,4,5,6,7,8,9,10,46,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,27,29,31,33,35,50,51,52,53,54,55,37,38,39,45,40,41,42,43,47,48,49,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,255};


const __FlashStringHelper *ithoHRUecoSettingsLabels[] =  {
    F("Percentage set 2 (%)"),
    F("Temp frost protection (°C)"),
    F("Frost temp offset (°K)"),
    F("Min frost speed (%)"),
    F("Max frost speed (%)"),
    F("Frost supply fan off time (Sec)"),
    F("Frost supply fan on time (Sec)"),
    F("Frost speed change ((%))"),
    F("Frostvalve speed change ((%))"),
    F("Max ratio exhaust / supply during frost protection (%)"),
    F("Summer temp (°C)"),
    F("Summer day time (K/Uur)"),
    F("Wanted temp bypass regulation (°C)"),
    F("Offset bypass regulation (K)"),
    F("Max bypass open time (Uur)"),
    F("Max rpm fast var. Pwm (rpm)"),
    F("Max rpm slow var. Pwm (rpm)"),
    F("Max rpm fast fixed pwm (rpm)"),
    F("Speed offset (rpm)"),
    F("High frequency var pwm (kHz)"),
    F("Low frequency var pwm (kHz)"),
    F("High frequency fixed pwm (kHz)"),
    F("Low frequentie fixed pwm (kHz)"),
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
    F("Manual operation"),
    F("Manual speed supply fan (rpm)"),
    F("Manual speed exhaust fan (rpm)"),
    F("Manual valve position (puls)"),
    F("Rpm supply fan with max position potmeter high (rpm)"),
    F("Supply fan of during vkk test"),
    F("Summer temp GHE (°C)"),
    F("Air quality"),
    F("Filter live time (month)"),
    F("Filter counter (hour)"),
    F("Min rpm supply fan (rpm)"),
    F("Max rpm supply fan (rpm)"),
    F("Min value balance setting (%)"),
    F("Max value balance setting (%)"),
    F("Min zeta supply value"),
    F("Max zeta supply value"),
    F(" Exhaust fan constant Ca2"),
    F("Exhaust fan constant Ca1"),
    F("Exhaust fan constant Cb0"),
    F("Exhaust fan constant Cb2"),
    F("Exhaust fan constant Cb1"),
    F("Exhaust fan constant Cc2"),
    F("Exhaust fan constant Cc1"),
    F("Exhaust fan constant Cc0"),
    F("Supply fan constant Ca2"),
    F("Supply fan constant Ca1"),
    F("Supply fan constant Cb0"),
    F("Supply fan constant Cb2"),
    F("Supply fan constant Cb1"),
    F("Supply fan constant Cc2"),
    F("Supply fan constant Cc1"),
    F("Supply fan constant Cc0")
};
const uint8_t itho_HRUecostatus2_4[] { 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,255};
const uint8_t itho_HRUecostatus6_7[] { 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,255};
const uint8_t itho_HRUecostatus8[] { 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,255};
const uint8_t itho_HRUecostatus10_12[] { 22,23,0,2,3,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,255};


const __FlashStringHelper *ithoHRUecoStatusLabels[] =  {
    F("Supply fan (%)"),
    F("Supply fan rpm (rpm)"),
    F("Supply fan actual (rpm)"),
    F("Exhaust fan (%)"),
    F("Exhaust fan rpm (rpm)"),
    F("Exhaust fan actual (rpm)"),
    F("Supply temp (°C)"),
    F("Exhaust temp (°C)"),
    F("Status"),
    F("Roomtemp (°C)"),
    F("Outdoortemp (°C)"),
    F("Valve position (pulse)"),
    F("Bypass position (pulse)"),
    F("Summercounter"),
    F("Summerday"),
    F("Frost timer (sec)"),
    F("Boiler timer (min)"),
    F("Startup counter"),
    F("Current position"),
    F("VKKswitch"),
    F("GHEswitch"),
    F("Airfilter counter"),
    F("Requested fanspeed (%)"),
    F("Balance (%)"),
};

const uint8_t *ithoHRUecoFanSettingsMap[] =   { nullptr, nullptr, itho_HRUecosetting2_3, itho_HRUecosetting2_3, itho_HRUecosetting4_6, nullptr, itho_HRUecosetting4_6, itho_HRUecosetting7, itho_HRUecosetting8, nullptr, itho_HRUecosetting10_12, itho_HRUecosetting10_12, itho_HRUecosetting10_12 };
const uint8_t *ithoHRUecoFanStatusMap[] =   { nullptr, nullptr, itho_HRUecostatus2_4, itho_HRUecostatus2_4, itho_HRUecostatus2_4, nullptr, itho_HRUecostatus6_7, itho_HRUecostatus6_7, itho_HRUecostatus8, nullptr, itho_HRUecostatus10_12, itho_HRUecostatus10_12, itho_HRUecostatus10_12 };

