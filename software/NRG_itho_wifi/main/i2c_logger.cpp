#include "i2c_logger.h"

I2CLogger i2cLogger;

// default constructor
I2CLogger::I2CLogger()
{
} // I2CLogger

// default destructor
I2CLogger::~I2CLogger()
{
} //~I2CLogger

// circular_buffer<i2c_log_t, 10> i2c_log;

int I2CLogger::i2c_log_start(i2c_cmdref_t cmd_ref)
{
    return i2c_log.put({
        .cmd_origin = cmd_ref,
        .cmd_start = millis(),
        .cmd_duration = 0,
        .error_state = EMPTY_STATE,
        .final_state = EMPTY_STATE,
    });
}

void I2CLogger::i2c_log_err_state(int log_entry_idx, i2c_error_state_t err_state)
{
    i2c_log.peek(log_entry_idx).error_state = err_state;
}

void I2CLogger::i2c_log_final(int log_entry_idx, i2c_error_state_t finalstate)
{
    i2c_log.peek(log_entry_idx).final_state = finalstate;
    i2c_log.peek(log_entry_idx).cmd_duration = millis() - i2c_log.peek(log_entry_idx).cmd_start;
}

const char *I2CLogger::i2c_cmdref_to_name(i2c_cmdref_t code) const
{
    size_t i;

    for (i = 0; i < sizeof(i2c_cmdref_msg_table) / sizeof(i2c_cmdref_msg_table[0]); ++i)
    {
        if (i2c_cmdref_msg_table[i].code == code)
        {
            return i2c_cmdref_msg_table[i].msg;
        }
    }
    return i2c_cmdref_unknown_msg;
}

const char *I2CLogger::i2c_error_state_to_name(i2c_error_state_t code) const
{
    size_t i;

    for (i = 0; i < sizeof(i2c_error_state_msg_table) / sizeof(i2c_error_state_msg_table[0]); ++i)
    {
        if (i2c_error_state_msg_table[i].code == code)
        {
            return i2c_error_state_msg_table[i].msg;
        }
    }
    return i2c_error_state_unknown_msg;
}

void I2CLogger::get(JsonObject obj, const char *rootName) const
{

    JsonArray nested = obj[rootName].to<JsonArray>();

    int current_last_entry = i2c_log.get_back_pos();

    for (int i = current_last_entry;; i--)
    {
        if (i == 0)
            i = I2CLOGGER_LOG_SIZE;

        JsonObject doc = nested.add<JsonObject>();

        i2c_log_t elem = i2c_log.peek(i);

        doc["origin"] = i2c_cmdref_to_name(elem.cmd_origin); // 32
        doc["start"] = elem.cmd_start;                       // 11
        doc["duration"] = elem.cmd_duration;
        doc["error"] = i2c_error_state_to_name(elem.error_state);
        doc["final"] = i2c_error_state_to_name(elem.final_state);

        if (current_last_entry + 1 == i)
            break;
    }
}