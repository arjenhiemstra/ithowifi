#pragma once

#include "error_info_labels.h"

const uint16_t itho_HRU200setting12_13[] 	{ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,999};
const uint16_t itho_HRU200setting14[] 		{ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,18,19,20,21,22,23,24,25,26,27,58,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,59,55,56,57,999};
const uint16_t itho_HRU200setting15[] 		{ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,18,19,20,21,22,23,24,25,26,27,58,60,61,62,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,59,55,56,57,999};
const uint16_t itho_HRU200setting17_18[] 	{ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,18,19,20,21,22,23,24,25,26,27,63,64,60,65,66,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,59,67,57,55,56,999};
const uint16_t itho_HRU200setting19_20[] 	{ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,18,19,20,21,22,23,24,25,26,27,63,64,60,65,66,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,59,67,57,68,69,70,71,55,56,999};
const uint16_t itho_HRU200setting21[] 		{ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,18,19,20,21,22,23,24,25,26,27,63,64,60,65,66,72,73,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,59,67,57,68,69,70,71,55,56,999};
const uint16_t itho_HRU200setting25[] 		{ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,18,19,20,21,22,23,24,25,26,27,63,64,74,65,66,72,73,75,76,77,78,79,80,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,59,67,57,68,69,70,71,55,56,999};
const uint16_t itho_HRU200setting26[] 		{ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,18,19,20,21,22,23,24,25,26,27,63,64,74,65,66,72,73,81,82,83,84,85,86,87,88,89,90,91,92,93,94,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,59,67,57,68,69,70,71,55,56,999};
const uint16_t itho_HRU200setting27[] 		{ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,18,19,20,21,22,23,24,25,26,27,63,64,74,65,66,72,73,81,82,83,84,85,86,87,88,89,90,91,92,93,94,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,59,67,57,68,69,70,71,95,96,97,98,99,55,56,999};


const char* ithoHRU200SettingsLabels[] =  {
    "OemNumber",
    "Unit version",
    "MinAdjustMinimumSpeed (rpm)",
    "MaxAdjustMinimumSpeed (rpm)",
    "MinAdjustMaximumSpeed (rpm)",
    "MaxAdjustMaximumSpeed (rpm)",
    "RFEnabled",
    "I2cIdle",
    "Constant_Ca2",
    "Constant_Ca1",
    "Constant_Ca0",
    "Constant_Cb2",
    "Constant_Cb1",
    "Constant_Cb0",
    "Constant_Cc2",
    "Constant_Cc1",
    "Constant_Cc0",
    "Speed in case of absence (rpm)",
    "Away wait time (min)",
    "Max delayed wait time (min)",
    "Delayed away level (%)",
    "PIR_Level1 (%)",
    "PIR_Level2 (%)",
    "PIR_Level1_waittime (sec)",
    "PIR_Level2_waittime (sec)",
    "PIR_RunOn_Level1 (sec)",
    "PIR_RunOn_Level2 (sec)",
    "Min ventlevel CO2 (%)",
    "AirQuality",
    "DefaultFilterLifeTime (Maand)",
    "FilterUsageCounter (uur)",
    "FrostProtectionSetPoint (°C)",
    "FrostOffset (K)",
    "FrostStepTime (sec)",
    "FrostStopTime (min)",
    "FrostDelayTime (min)",
    "FrostSpeed (tpm)",
    "FrostSpeedAdaptSpeed (rpm)",
    "MaximumSpeedFrostTime (min)",
    "ByPassControl",
    "ByPassCommissionTime (min)",
    "ByPassControlTime (min)",
    "ByPassCalibrationValue",
    "ByPassValveCleanTimer (uur)",
    "ByPassValvePosition0% (steps)",
    "ByPassValvePosition100% (steps)",
    "ByPassValvePositionMax (steps)",
    "ByPassValvePositionMin (steps)",
    "ByPassValveExtraSteps (steps)",
    "RoomTemperatureSetpoint (°C)",
    "OffsetRoomTempCloseSetpoint (K)",
    "OffsetRoomTempOpenSetpoint (K)",
    "AveragingTimeOutLetTemp (min)",
    "OffsetColdRecovery (K)",
    "ByPassValveNrOfPositions",
    "ManualControl",
    "ManualFanSpeed (rpm)",
    "ByPassValveSetManualPos (steps)",
    "Maximum time mode high (min)",
    "ByPassValveRotationSlack (steps)",
    "Position after max. time high",
    "Moderate CO2 concentration (ppm)",
    "Good CO2 concentration (ppm)",
    "MaxTimeHighSpeed (min)",
    "MaxTimeOtherSpeeds (min)",
    "PoorCo2Quality (ppm)",
    "GoodCo2Quality (ppm)",
    "ByPassBlocktime (min)",
    "FrostValveSpeed3 (%)",
    "FrostValveSpeed1 (%)",
    "FrostValveAdapt (%)",
    "FrostValveError (K)",
    "Amount of Floors",
    "Inhabitants",
    "FallbackPreviousSpeed",
    "NightOnePersonOneFloor (%)",
    "NightTwoPersonsOneFloor (%)",
    "NightMultiPersonsOneFloor (%)",
    "NightOnePersonMultiFloor (%)",
    "NightTwoPersonsMultiFloor (%)",
    "NightMultiPersonsMultiFloor (%)",
    "NightOnePersonOneFloorOptima1 (%)",
    "NightTwoPersonsOneFloorOptima1 (%)",
    "NightMultiPersonsOneFloorOptima1 (%)",
    "NightOnePersonMultiFloorOptima1 (%)",
    "NightTwoPersonsMultiFloorOptima1 (%)",
    "NightMultiPersonsMultiFloorOptima1 (%)",
    "NightOnePersonOneFloorOptima2 (%)",
    "NightTwoPersonsOneFloorOptima2 (%)",
    "NightMultiPersonsOneFloorOptima2 (%)",
    "NightOnePersonMultiFloorOptima2 (%)",
    "NightTwoPersonsMultiFloorOptima2 (%)",
    "NightMultiPersonsMultiFloorOptima2 (%)",
    "AutoVentilationOneFloor (%)",
    "AutoVentilationMultiFloor (%)",
    "SummerNightBoost",
    "SummerNightDegree (Kh)",
    "SummerNightSetpoint (°C)",
    "SummerNightSpeed (%)",
    "SummerNightMaxAreaDegreeHours (Kh)",
};


const uint8_t itho_HRU200status12_13[] 	{ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,255};
const uint8_t itho_HRU200status14[] 	{ 0,1,2,3,4,5,6,7,8,23,9,10,11,12,13,14,15,16,17,18,19,20,21,22,255};
const uint8_t itho_HRU200status15_17[] 	{ 0,1,2,3,4,5,6,7,8,23,24,9,10,11,12,13,14,15,16,17,18,19,20,21,22,255};
const uint8_t itho_HRU200status18[] 	{ 0,1,2,3,4,5,6,7,8,23,24,9,10,11,12,13,14,15,16,17,18,19,20,21,22,25,255};
const uint8_t itho_HRU200status19[] 	{ 0,1,2,3,4,5,6,7,8,23,24,9,10,11,12,13,14,15,16,17,18,19,20,21,22,25,26,27,255};
const uint8_t itho_HRU200status20_27[] 	{ 0,1,2,3,4,5,6,7,23,24,9,10,11,12,13,14,15,17,18,19,20,22,25,26,27,255};


const struct ithoLabels ithoHRU200StatusLabels[] {
	{ "Ventilation setpoint (%)", 					"ventilation-setpoint_perc" },
	{ "Speed setpoint (rpm)", 						"speed-setpoint_rpm" },
	{ "Actual speed (rpm)", 						"speed-actual_rpm" },
	{ "Error", 										"error" },
	{ "Selection", 									"selection" },
	{ "Start-up counter", 							"start-up-counter" },
	{ "Total operation (hrs)", 						"total-operation" },
	{ "Absence (min)", 								"absence_min" },
	{ "Deferred absence (min)", 					"deferred-absence_min" },
	{ "Filter use counter (hour)", 					"filter-use-counter_hour" },
	{ "Frost status", 								"frost-status" },
	{ "Fan off during frost (min)", 				"fan-off-during-frost_min" },
	{ "Duration of current frost setpoint (sec)", 	"duration-actual-frost-setpoint_sec" },
	{ "Frost adjustment speed (rpm)", 				"frost-adjustment-speed_rpm" },
	{ "Outside temperature (°C)", 					"outside-temperature_c" },
	{ "Air discharge temperature (°C)", 			"air-discharge-temperature_c" },
	{ "Average air outlet temperature (°C)", 		"average-air outlet-temperature_c" },
	{ "Room temp setpoint (°C)", 					"room-temp-setpoint_c" },
	{ "Bypass mode", 								"bypass-mode" },
	{ "Bypass setpoint (%)", 						"bypass-setpoint_perc" },
	{ "Periodic bypass valve travel (min)",			"periodic-bypass-valve_min" },
	{ "Number of bypass settings", 					"number-of-bypass-settings" },
	{ "Current bypass share (%)", 					"current-bypass-share_perc" },
	{ "Max. CO2 level (ppm)", 						"max-co2-nivo_ppm" },
	{ "Max. RH level (%)", 							"max-rv-nivo_perc" },
	{ "Block timer bypass (min)", 					"block-timer-bypass_min" },
	{ "Position of frost valve (%)", 				"position-of-frost-valve_perc" },
	{ "Max frost vent. speed (rpm)", 				"max-frost-vent-speed_rpm" }
};



const uint16_t *ithoHRU200SettingsMap[] =      	{ nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, itho_HRU200setting12_13, itho_HRU200setting12_13, itho_HRU200setting14, itho_HRU200setting15, nullptr, itho_HRU200setting17_18, itho_HRU200setting17_18, itho_HRU200setting19_20, itho_HRU200setting19_20, itho_HRU200setting21, nullptr, nullptr, nullptr, itho_HRU200setting25, itho_HRU200setting26, itho_HRU200setting27 };
const uint8_t  *ithoHRU200StatusMap[] =      	{ nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, itho_HRU200status12_13, itho_HRU200status12_13, itho_HRU200status14, itho_HRU200status15_17, nullptr, itho_HRU200status15_17, itho_HRU200status18, itho_HRU200status19, itho_HRU200status20_27, itho_HRU200status20_27, nullptr, nullptr, nullptr, itho_HRU200status20_27, itho_HRU200status20_27, itho_HRU200status20_27 };

