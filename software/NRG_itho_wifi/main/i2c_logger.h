#pragma once

#define I2CLOGGER_LOG_SIZE 15

#include "cirbuf.hpp"
#include <Arduino.h>
#include <ArduinoJson.h>

typedef enum
{
    I2C_CMD_EMPTY_CMDREF,
    I2C_CMD_UNKOWN,
    I2C_CMD_PWM_INIT,
    I2C_CMD_PWM_CMD,
    I2C_CMD_REMOTE_CMD,
    I2C_CMD_QUERY_DEVICE_TYPE,
    I2C_CMD_QUERY_STATUS_FORMAT,
    I2C_CMD_QUERY_STATUS,
    I2C_CMD_QUERY_31DA,
    I2C_CMD_QUERY_31D9,
    I2C_CMD_QUERY_2410,
    I2C_CMD_SET_2410,
    I2C_CMD_SET_CE30,
    I2C_CMD_FILTER_RESET,
    I2C_CMD_TEMP_READ_ITHO,
    I2C_CMD_TEMP_READ_SLAVE,
    I2C_CMD_TEMP_READ_CMD,
    I2C_CMD_OTHER
} i2c_cmdref_t;

class I2CLogger
{
public:
    typedef enum
    {
        EMPTY_STATE,
        I2C_OK,
        I2C_NOK,
        I2C_ERROR_SDA_LOW,
        I2C_ERROR_SCL_LOW,
        I2C_ERROR_SCL_SDA_LOW,
        I2C_ERROR_SLAVE_CLOCKSTRETCH,
        I2C_ERROR_REQEUST_FAILED,
        I2C_ERROR_MASTER_INIT_FAIL,
        I2C_ERROR_LEN,
        I2C_ERROR_CLEARED_BUS,
        I2C_ERROR_CLEARED_OK,
        I2C_ERROR_UNKNOWN
    } i2c_error_state_t;

private:
    typedef struct I2c_log
    {
        i2c_cmdref_t cmd_origin;
        unsigned long cmd_start;
        unsigned long cmd_duration;
        i2c_error_state_t error_state;
        i2c_error_state_t final_state;
    } i2c_log_t;

    typedef struct
    {
        i2c_cmdref_t code;
        const char *msg;
    } i2c_cmdref_msg;

    typedef struct
    {
        i2c_error_state_t code;
        const char *msg;
    } i2c_error_state_msg;

    const i2c_cmdref_msg i2c_cmdref_msg_table[18]{
        {I2C_CMD_EMPTY_CMDREF, ""},
        {I2C_CMD_UNKOWN, "UNKOWN"},
        {I2C_CMD_PWM_INIT, "PWM_INIT"},
        {I2C_CMD_PWM_CMD, "PWM_CMD"},
        {I2C_CMD_REMOTE_CMD, "REMOTE_CMD"},
        {I2C_CMD_QUERY_DEVICE_TYPE, "QUERY_DEVICE_TYPE"},
        {I2C_CMD_QUERY_STATUS_FORMAT, "QUERY_STATUS_FORMAT"},
        {I2C_CMD_QUERY_STATUS, "QUERY_STATUS"},
        {I2C_CMD_QUERY_31DA, "QUERY_31DA"},
        {I2C_CMD_QUERY_31D9, "QUERY_31D9"},
        {I2C_CMD_QUERY_2410, "QUERY_2410"},
        {I2C_CMD_SET_2410, "SET_2410"},
        {I2C_CMD_SET_CE30, "SET_CE30"},
        {I2C_CMD_FILTER_RESET, "FILTER_RESET"},
        {I2C_CMD_TEMP_READ_ITHO, "TEMP_READ_FROM_ITHO"},
        {I2C_CMD_TEMP_READ_SLAVE, "TEMP_READ_SLAVE"},
        {I2C_CMD_TEMP_READ_CMD, "TEMP_READ_CMD"},
        {I2C_CMD_OTHER, "I2C_CMD_OTHER"}};
    const char *i2c_cmdref_unknown_msg = "UNKNOWN ERROR";

    const i2c_error_state_msg i2c_error_state_msg_table[13]{
        {EMPTY_STATE, ""},
        {I2C_OK, "I2C_OK"},
        {I2C_NOK, "I2C_NOK"},
        {I2C_ERROR_SDA_LOW, "I2C_ERROR_SDA_LOW"},
        {I2C_ERROR_SCL_LOW, "I2C_ERROR_SCL_LOW"},
        {I2C_ERROR_SCL_SDA_LOW, "I2C_ERROR_SCL_SDA_LOW"},
        {I2C_ERROR_SLAVE_CLOCKSTRETCH, "I2C_ERROR_SLAVE_CLOCKSTRETCH"},
        {I2C_ERROR_REQEUST_FAILED, "I2C_ERROR_REQEUST_FAILED"},
        {I2C_ERROR_MASTER_INIT_FAIL, "I2C_ERROR_MASTER_INIT_FAIL"},
        {I2C_ERROR_LEN, "I2C_ERROR_LEN"},
        {I2C_ERROR_CLEARED_BUS, "I2C_ERROR_CLEARED_BUS"},
        {I2C_ERROR_CLEARED_OK, "I2C_ERROR_CLEARED_OK"},
        {I2C_ERROR_UNKNOWN, "I2C_ERROR_UNKNOWN"}};
    const char *i2c_error_state_unknown_msg = "UNKNOWN ERROR";

    circular_buffer<i2c_log_t, I2CLOGGER_LOG_SIZE> i2c_log;

    const char *i2c_cmdref_to_name(i2c_cmdref_t code) const;
    const char *i2c_error_state_to_name(i2c_error_state_t code) const;

public:
    I2CLogger();
    ~I2CLogger();

    int i2c_log_start(i2c_cmdref_t cmd_ref);
    void i2c_log_err_state(int log_entry_idx, i2c_error_state_t err_state);
    void i2c_log_final(int log_entry_idx, i2c_error_state_t finalstate);
    void get(JsonObject obj, const char *rootName) const;

protected:
}; // I2CLogger

extern I2CLogger i2cLogger;
