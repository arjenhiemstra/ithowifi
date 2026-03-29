#include "tasks/task_syscontrol.h"
#include "tasks/task_configandlog.h"

bool IthoInit = false;
int8_t ithoInitResult = 0;
bool ithoStatusFormateSuccessful = false;

unsigned long lastI2CinitRequest = 0;
bool ithoInitResultLogEntry = true;
bool i2c_init_functions_done = false;
bool joinSend = false;

void initI2cFunctions()
{
  if (millis() > I2C_INIT_DELAY_MS && (millis() - lastI2CinitRequest > I2C_INIT_RETRY_INTERVAL_MS))
  {
    lastI2CinitRequest = millis();
    sendQueryDevicetype(false);

    if (currentItho_fwversion() > 0)
    {
      N_LOG("I2C: QueryDevicetype - mfr:0x%02X type:0x%02X fw:0x%02X hw:0x%02X", currentIthoDeviceGroup(), currentIthoDeviceID(), currentItho_fwversion(), currentItho_hwversion());

      ithoInitResult = 1;
      i2c_init_functions_done = true;

      // Auto-set RF TX power for Send remotes based on device type (only for remotes still at default 0xC0)
      // CVE/HRU200 (0x1B, 0x1D, 0x14, 0x04): add-on inside unit ~1cm → -30dBm (0x03)
      // Other I2C devices: ~30cm → +5dBm (0x84)
      {
        uint8_t devId = currentIthoDeviceID();
        uint8_t defaultPower = 0xC0;
        if (devId == 0x1B || devId == 0x1D || devId == 0x14 || devId == 0x04)
          defaultPower = 0x03; // -30 dBm for CVE/HRU200
        else if (devId != 0)
          defaultPower = 0x84; // +5 dBm for other I2C devices

        bool changed = false;
        for (int idx = 0; idx < remotes.getMaxRemotes(); idx++)
        {
          if (remotes.isEmptySlot(idx)) continue;
          if (remotes.getRemoteFunction(idx) == RemoteFunctions::SEND && remotes.getRemoteTxPower(idx) == 0xC0)
          {
            remotes.updateRemoteTxPower(idx, defaultPower);
            changed = true;
          }
        }
        if (changed)
        {
          saveRemotesflag = true;
        }
        N_LOG("RF: TX power 0x%02X for Send remotes (device type 0x%02X)", defaultPower, devId);
      }

      digitalWrite(hardwareManager.status_pin, HIGH);
      if (systemConfig.rfInitOK)
      {
        delay(250);
        digitalWrite(hardwareManager.status_pin, LOW);
        delay(250);
        digitalWrite(hardwareManager.status_pin, HIGH);
        delay(250);
        digitalWrite(hardwareManager.status_pin, LOW);
        delay(250);
        digitalWrite(hardwareManager.status_pin, HIGH);
      }
      if (systemConfig.syssht30 > 0)
      {
        if (currentIthoDeviceID() == ITHO_DEVICE_CVE_SILENT)
        {
          uint8_t index = 0;
          int32_t value = 0;

          if (systemConfig.syssht30 == 1)
          {
            /*
               switch itho hum setting off on every boot because
               hum sensor setting gets restored in itho firmware after power cycle
            */
            value = 0;
          }

          else if (systemConfig.syssht30 == 2)
          {
            /*
               if value == 2 setting changed from 1 -> 0
               then restore itho hum setting back to "on"
            */
            value = 1;
          }
          if (currentItho_fwversion() == 25)
          {
            index = 63;
          }
          else if (currentItho_fwversion() == 26 || currentItho_fwversion() == 27)
          {
            index = 71;
          }
          if (index > 0)
          {
            i2cQueueAddCmd([index]()
                              { sendQuery2410(index, false); });
            i2cQueueAddCmd([index, value]()
                              { setSetting2410(index, value, false); });

            N_LOG("I2C: set hum sensor in itho firmware to: %s", value ? "on" : "off");
          }
        }
        if (systemConfig.syssht30 == 2)
        {
          systemConfig.syssht30 = 0;
          N_LOG("I2C: hum sensor setting set to \"off\"");
          saveSystemConfig("flash");
        }
      }

      sendQueryStatusFormat(false);
      N_LOG("I2C: QueryStatusFormat - items:%lu", static_cast<unsigned long>(ithoStatus.size()));
      if (ithoStatus.size() > 0)
        ithoStatusFormateSuccessful = true;

      if (systemConfig.itho_2401 == 1)
      {
        sendQueryStatus(false);
        N_LOG("I2C: initial QueryStatus");
      }
      if (systemConfig.itho_31da == 1)
      {
        sendQuery31DA(false);
        N_LOG("I2C: initial Query31DA");
      }
      if (systemConfig.itho_31d9 == 1)
      {
        sendQuery31D9(false);
        N_LOG("I2C: initial Query31D9");
      }
      if (systemConfig.itho_4210 == 1)
      {
        sendQueryCounters(false);
        N_LOG("I2C: initial QueryCounters");
      }

      if (systemConfig.i2c_safe_guard > 0 && currentIthoDeviceID() == 0x1B)
      {
        if (hardwareManager.i2c_sniffer_capable)
        {
          auto *sg = static_cast<i2c_safe_guard_t *>(i2cManager.safe_guard);
          if (sg == nullptr)
          {
            E_LOG("I2C: safe guard pointer is null");
          }
          else if (systemConfig.syssht30 == 0 && currentItho_fwversion() >= 25)
          {
            N_LOG("I2C: safe guard enabled");
            sg->i2c_safe_guard_enabled = true;
            i2cSnifferEnable();
          }
          else
          {
            if (systemConfig.i2c_safe_guard == 1)
            {
              W_LOG("I2C: i2c safe guard could not be enabled");
            }
          }
        }
        else
        {
          if (systemConfig.i2c_safe_guard == 1)
          {
            E_LOG("I2C: i2c safe guard needs sniffer capable hardware");
          }
        }
      }
      else
      {
        if (systemConfig.i2c_safe_guard == 1)
        {
          E_LOG("I2C init: i2c safe guard needs supported itho device to work");
        }
      }
      if (systemConfig.i2c_sniffer == 1)
      {
        if (!hardwareManager.i2c_sniffer_capable)
        {
          E_LOG("I2C init: i2c sniffer needs sniffer capable hardware");
        }
        else
        {
          auto *sg = static_cast<i2c_safe_guard_t *>(i2cManager.safe_guard);
          N_LOG("I2C: i2c sniffer enabled");
          sg->sniffer_enabled = true;
          i2cSnifferEnable();
        }
      }
      if (HADiscConfigLoaded && (strcmp(haDiscConfig.d, "unset") == 0))
      {
        strlcpy(haDiscConfig.d, getIthoType(), sizeof(haDiscConfig.d));
      }
      sendHomeAssistantDiscovery = true;
    }
    else
    {
      if (ithoInitResultLogEntry)
      {
        if (ithoInitResult != -2)
        {
          ithoInitResult = -1;
        }
        ithoInitResultLogEntry = false;
        E_LOG("I2C: QueryDevicetype - failed");
      }
    }
  }
}

void init_vRemote()
{
  // setup virtual remote
  I_LOG("SYS: Virtual remotes, start ID: %02X,%02X,%02X - No.: %d", sys.getMac(3), sys.getMac(4), sys.getMac(5), systemConfig.itho_numvrem);

  virtualRemotes.setMaxRemotes(systemConfig.itho_numvrem);
  VirtualRemotesConfigLoaded = loadVirtualRemotesConfig("flash");
}

bool ithoInitCheck()
{
  if (hardwareManager.hardware_rev_det == 0x3F || hardwareManager.hardware_rev_det == 0x03) // CVE
  {
    if (digitalRead(hardwareManager.status_pin) == LOW)
    {
      return false;
    }
    if (systemConfig.itho_pwm2i2c)
    {
      sendI2CPWMinit();
      // sendCO2init();
    }
  }
  return false;
}
