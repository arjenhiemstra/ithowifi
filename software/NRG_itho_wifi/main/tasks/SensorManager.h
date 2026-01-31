#pragma once

// Sensor state globals (moved from task_syscontrol.cpp)
extern bool SHT3x_original;
extern bool SHT3x_alternative;
extern bool reset_sht_sensor;

// Sensor functions (moved from task_syscontrol.h)
void initSensor();
void updateShtSensor();
