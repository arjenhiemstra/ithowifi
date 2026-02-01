#include "tasks/task_syscontrol.h"

bool SHT3x_original = false;
bool SHT3x_alternative = false;
bool reset_sht_sensor = false;

SHTSensor sht_org(SHTSensor::SHT3X);
SHTSensor sht_alt(SHTSensor::SHT3X_ALT);

void initSensor()
{

  if (systemConfig.syssht30 == 1)
  {

    if (sht_org.init() && sht_org.readSample())
    {
      SHT3x_original = true;
    }
    else if (sht_alt.init() && sht_alt.readSample())
    {
      SHT3x_alternative = true;
    }
    if (SHT3x_original)
    {
      N_LOG("SYS: original SHT30 sensor found");
    }
    else if (SHT3x_alternative)
    {
      N_LOG("SYS: alternative SHT30 sensor found");
    }
    if (SHT3x_original || SHT3x_alternative)
    {
      return;
    }

    delay(200);

    if (sht_org.init() && sht_org.readSample())
    {
      SHT3x_original = true;
    }
    else if (sht_alt.init() && sht_alt.readSample())
    {
      SHT3x_alternative = true;
    }
    if (SHT3x_original)
    {
      W_LOG("SYS: original SHT30 sensor found (2nd try)");
    }
    else if (SHT3x_alternative)
    {
      W_LOG("SYS: alternative SHT30 sensor found (2nd try)");
    }
    else
    {
      systemConfig.syssht30 = 0;
      N_LOG("SYS: SHT30 sensor not found");
    }
  }
}

void updateShtSensor()
{

  if (SHT3x_original)
  {
    if (reset_sht_sensor)
    {
      reset_sht_sensor = false;
      bool reset_res = sht_org.softReset();
      if (reset_res)
      {
        ithoInitResult = 0;
        ithoInitResultLogEntry = true;
        i2c_init_functions_done = false;
        sysStatReq = true;
      }
      N_LOG("SYS: SHT3x sensor reset: %s", reset_res ? "Success" : "Failed");
      char buf[32]{};
      snprintf(buf, sizeof(buf), "%s", reset_res ? "OK" : "NOK");
      jsonSysmessage("i2c_sht_reset", buf);
    }
    else if (sht_org.readSample())
    {
      ithoHum = sht_org.getHumidity();
      ithoTemp = sht_org.getTemperature();
    }
  }
  if (SHT3x_alternative)
  {
    if (reset_sht_sensor)
    {
      reset_sht_sensor = false;
      bool reset_res = sht_alt.softReset();
      if (reset_res)
      {
        ithoInitResult = 0;
        ithoInitResultLogEntry = true;
        i2c_init_functions_done = false;
        sysStatReq = true;
      }
      N_LOG("SYS: SHT3x sensor reset: %s", reset_res ? "Success" : "Failed");
      char buf[32]{};
      snprintf(buf, sizeof(buf), "%s", reset_res ? "OK" : "NOK");
      jsonSysmessage("i2c_sht_reset", buf);
    }
    else if (sht_alt.readSample())
    {
      ithoHum = sht_alt.getHumidity();
      ithoTemp = sht_alt.getTemperature();
    }
  }
}
