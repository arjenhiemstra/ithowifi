#pragma once

// I2C init state globals (moved from task_syscontrol.cpp)
extern bool IthoInit;
extern int8_t ithoInitResult;
extern bool ithoStatusFormateSuccessful;
extern bool i2c_init_functions_done;
extern bool ithoInitResultLogEntry;
extern bool joinSend;

// I2C init functions (moved from task_syscontrol.h)
void initI2cFunctions();
bool ithoInitCheck();
void init_vRemote();
