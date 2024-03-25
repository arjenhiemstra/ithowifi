#include "tasks/task_mqtt.h"

#define TASK_MQTT_PRIO 5

// globals
TaskHandle_t xTaskMQTTHandle = NULL;
uint32_t TaskMQTTHWmark = 0;
PubSubClient mqttClient(client);

// locals
StaticTask_t xTaskMQTTBuffer;
StackType_t xTaskMQTTStack[STACK_SIZE_MEDIUM];
bool sendHomeAssistantDiscovery = false;
bool updateIthoMQTT = false;

Ticker TaskMQTTTimeout;

#include <cmath>

void startTaskMQTT()
{
  xTaskMQTTHandle = xTaskCreateStaticPinnedToCore(
      TaskMQTT,
      "TaskMQTT",
      STACK_SIZE_MEDIUM,
      (void *)1,
      TASK_MQTT_PRIO,
      xTaskMQTTStack,
      &xTaskMQTTBuffer,
      CONFIG_ARDUINO_RUNNING_CORE);
}

void TaskMQTT(void *pvParameters)
{
  configASSERT((uint32_t)pvParameters == 1UL);

  mqttInit();

  startTaskWeb();

  esp_task_wdt_add(NULL);

  for (;;)
  {
    yield();
    esp_task_wdt_reset();

    TaskMQTTTimeout.once_ms(35000UL, []()
                            { W_LOG("Warning: Task MQTT timed out!"); });

    execMQTTTasks();

    TaskMQTTHWmark = uxTaskGetStackHighWaterMark(NULL);
    vTaskDelay(25 / portTICK_PERIOD_MS);
  }

  TaskHandle_t xTempTask = xTaskMQTTHandle;
  xTaskMQTTHandle = NULL;
  vTaskDelete(xTempTask);
}

void execMQTTTasks()
{

  if (systemConfig.mqtt_updated)
  {
    systemConfig.mqtt_updated = false;
    setupMQTTClient();
  }
  // handle MQTT:
  if (systemConfig.mqtt_active == 0)
  {
    MQTT_conn_state_new = -5;
  }
  else
  {
    MQTT_conn_state_new = mqttClient.state();
  }
  if (MQTT_conn_state != MQTT_conn_state_new)
  {
    MQTT_conn_state = MQTT_conn_state_new;
    sysStatReq = true;
  }
  if (mqttClient.connected())
  {
    if (sendHomeAssistantDiscovery)
    {
      mqttHomeAssistantDiscovery();
    }
    if (updateIthoMQTT)
    {
      updateIthoMQTT = false;
      updateState(ithoCurrentVal);
      mqttPublishLastcmd();
    }
    // if (updateMQTTmodeStatus)
    // {
    //   updateMQTTmodeStatus = false;
    //   if (mqttClient.connected())
    //   {
    //     char modestatetopic[140]{};
    //     snprintf(modestatetopic, sizeof(modestatetopic), "%s%s", systemConfig.mqtt_base_topic, "/modestate");
    //     mqttClient.publish(modestatetopic, modestate, true);
    //   }
    // }
    if (updateMQTTihtoStatus)
    {
      updateMQTTihtoStatus = false;

      if (mqttClient.connected())
      {

        if (systemConfig.mqtt_domoticz_active)
        {
          char buffer[512]{};
          auto humstat = 0;
          // Humidity Status
          if (ithoHum < 31)
          {
            humstat = 2;
          }
          else if (ithoHum > 69)
          {
            humstat = 3;
          }
          else if (ithoHum > 34 && ithoHum < 66 && ithoTemp > 21 && ithoTemp < 27)
          {
            humstat = 1;
          }

          char svalue[32]{};
          snprintf(svalue, sizeof(svalue), "%1.1f;%1.1f;%d", ithoTemp, ithoHum, humstat);

          JsonDocument root;
          root["svalue"] = svalue;
          root["nvalue"] = 0;
          root["idx"] = systemConfig.sensor_idx;
          serializeJson(root, buffer);
          mqttClient.publish(systemConfig.mqtt_domoticzin_topic, buffer, true);
        }
        mqttSendStatus();
        mqttSendRemotesInfo();
        mqttPublishLastcmd();
        // mqttSendSettingsJSON();
      }
    }
    mqttClient.loop();
  }
  else
  {
    if (dontReconnectMQTT)
      return;
    if (millis() - lastMQTTReconnectAttempt > 5000)
    {

      lastMQTTReconnectAttempt = millis();
      // Attempt to reconnect
      if (reconnect())
      {
        sendHomeAssistantDiscovery = true;
        lastMQTTReconnectAttempt = 0;
      }
    }
  }
}

void mqttInit()
{
  if (systemConfig.mqtt_active)
  {
    if (!setupMQTTClient())
    {
      E_LOG("MQTT: connection failed, System config: %d", systemConfig.mqtt_active);
    }
    else
    {
      N_LOG("MQTT: connected, System config: %d", systemConfig.mqtt_active);
    }
  }
}

void mqttSendStatus()
{

  char ihtostatustopic[140]{};
  snprintf(ihtostatustopic, sizeof(ihtostatustopic), "%s%s", systemConfig.mqtt_base_topic, "/ithostatus");

  JsonDocument doc;
  JsonObject root = doc.to<JsonObject>();
  getIthoStatusJSON(root);

  size_t len = measureJson(root);

  if (mqttClient.getBufferSize() < len)
  {
    mqttClient.setBufferSize(len);
  }
  if (mqttClient.beginPublish(ihtostatustopic, len, true))
  {
    serializeJson(root, mqttClient);
    if (!mqttClient.endPublish())
      E_LOG("MQTT: Failed to send payload (itho status)");
  }
  if (doc.overflowed())
  {
    E_LOG("MQTT: JsonDocument overflowed (itho status)");
  }

  mqttClient.setBufferSize(MQTT_BUFFER_SIZE);
}
void mqttSendRemotesInfo()
{
  char remotesinfotopic[140]{};
  snprintf(remotesinfotopic, sizeof(remotesinfotopic), "%s%s", systemConfig.mqtt_base_topic, "/remotesinfo");

  JsonDocument doc;
  JsonObject root = doc.to<JsonObject>();

  getRemotesInfoJSON(root);

  size_t len = measureJson(root);

  if (mqttClient.getBufferSize() < len)
  {
    mqttClient.setBufferSize(len);
  }
  if (mqttClient.beginPublish(remotesinfotopic, len, true))
  {
    serializeJson(root, mqttClient);
    if (!mqttClient.endPublish())
      E_LOG("MQTT: Failed to send payload (itho remote info))");
  }
  if (doc.overflowed())
  {
    E_LOG("MQTT: JsonDocument overflowed (itho remote info)");
  }

  mqttClient.setBufferSize(MQTT_BUFFER_SIZE);
}

void mqttPublishLastcmd()
{
  char lastcmdtopic[140]{};
  snprintf(lastcmdtopic, sizeof(lastcmdtopic), "%s%s", systemConfig.mqtt_base_topic, "/lastcmd");
  JsonDocument doc;

  JsonObject root = doc.to<JsonObject>();

  getLastCMDinfoJSON(root);

  size_t len = measureJson(root);

  if (mqttClient.getBufferSize() < len)
  {
    mqttClient.setBufferSize(len);
  }
  if (mqttClient.beginPublish(lastcmdtopic, len, true))
  {
    serializeJson(root, mqttClient);
    if (!mqttClient.endPublish())
      E_LOG("MQTT: Failed to send payload (last cmd info))");
  }
  // reset buffer
  mqttClient.setBufferSize(MQTT_BUFFER_SIZE);
}

void mqttSendSettingsJSON()
{
  JsonDocument doc;

  JsonObject root = doc.to<JsonObject>();

  getIthoSettingsBackupJSON(root);

  size_t len = measureJson(root);

  if (mqttClient.getBufferSize() < len)
  {
    mqttClient.setBufferSize(len);
  }
  if (mqttClient.beginPublish("itho/settingstest", len, true))
  {
    serializeJson(root, mqttClient);
    if (!mqttClient.endPublish())
      E_LOG("MQTT: Failed to send payload (itho remote info))");
  }
  // reset buffer
  mqttClient.setBufferSize(MQTT_BUFFER_SIZE);
}

void mqttCallback(const char *topic, const byte *payload, unsigned int length)
{

  if (topic == NULL)
    return;
  if (payload == NULL)
    return;

  if (length > 1023)
    length = 1023;

  char s_payload[length];
  std::memcpy(s_payload, payload, length);
  s_payload[length] = '\0';

  char c_topic[140]{};
  snprintf(c_topic, sizeof(c_topic), "%s%s", systemConfig.mqtt_base_topic, "/cmd");

  if (strcmp(topic, c_topic) == 0)
  {
    JsonDocument root;
    DeserializationError error = deserializeJson(root, s_payload);

    if (!error)
    {
      bool jsonCmd = false;
      // if (!root["idx"].isNull())
      // {
      //   jsonCmd = true;
      //   // printf("JSON parse -- idx match");
      //   uint16_t idx = root["idx"].as<uint16_t>();
      //   if (idx == systemConfig.mqtt_idx)
      //   {
      //     if (!root["svalue1"].isNull())
      //     {
      //       uint16_t invalue = root["svalue1"].as<uint16_t>();
      //       float value = invalue * 2.54;
      //       ithoSetSpeed((uint16_t)value, MQTTAPI);
      //     }
      //   }
      // }
      // if (!root["dtype"].isNull())
      // {
      //   const char *value = root["dtype"] | "";
      //   if (strcmp(value, "ithofan") == 0)
      //   {
      //     dtype = true;
      //   }
      // }

      /*
         standard true, unless mqtt_domoticz_active == "on"
         if mqtt_domoticz_active == "on"
            this should be set to true first by a JSON containing key:value pair "dtype":"ithofan",
            otherwise different commands might get processed due to domoticz general domoticz/out topic structure
      */
      if (!root["command"].isNull())
      {
        jsonCmd = true;
        const char *value = root["command"] | "";
        ithoExecCommand(value, MQTTAPI);
      }
      if (!root["vremote"].isNull() || !root["vremotecmd"].isNull())
      {
        const char *command = root["vremote"] | "";
        if (strcmp(command, "") == 0)
          command = root["vremotecmd"] | "";

        if (!root["vremoteindex"].isNull() && !root["vremotename"].isNull())
        {
          jsonCmd = true;
          ithoI2CCommand(0, command, MQTTAPI);
        }
        else
        {
          int index = -1;
          if (!root["vremotename"].isNull())
          {
            index = virtualRemotes.getRemoteIndexbyName((const char *)root["vremotename"]);
          }
          else
          {
            index = root["vremoteindex"];
          }
          if (index >= 0)
          {
            jsonCmd = true;
            ithoI2CCommand(index, command, MQTTAPI);
          }
        }
      }
      if (!root["rfremotecmd"].isNull() || !root["rfremoteindex"].isNull())
      {
        uint8_t idx = 0;
        if (!root["rfremoteindex"].isNull())
        {
          idx = strtoul(root["rfremoteindex"], NULL, 10);
        }
        if (!root["rfremotecmd"].isNull())
        {
          jsonCmd = true;
          ithoExecRFCommand(idx, root["rfremotecmd"], MQTTAPI);
        }
      }
      if (!root["speed"].isNull())
      {
        jsonCmd = true;
        if (!root["timer"].isNull())
        {
          ithoSetSpeedTimer(root["speed"].as<uint16_t>(), root["timer"].as<uint16_t>(), MQTTAPI);
        }
        else
        {
          ithoSetSpeed(root["speed"].as<uint16_t>(), MQTTAPI);
        }
      }
      else if (!root["timer"].isNull())
      {
        jsonCmd = true;
        ithoSetTimer(root["timer"].as<uint16_t>(), MQTTAPI);
      }
      if (!root["clearqueue"].isNull())
      {
        jsonCmd = true;
        const char *value = root["clearqueue"] | "";
        if (strcmp(value, "true") == 0)
        {
          clearQueue = true;
        }
      }
      if (!(const char *)root["outside_temp"].isNull())
      {
        jsonCmd = true;
        float outside_temp = root["outside_temp"].as<float>();
        float temporary_outside_temp = root["temporary_outside_temp"].as<float>();
        uint32_t valid_until = root["valid_until"].as<uint32_t>();
        setSettingCE30(static_cast<int16_t>(temporary_outside_temp * 100), static_cast<int16_t>(outside_temp * 100), valid_until, false);
      }
      if (!(const char *)root["manual_operation_index"].isNull())
      {
        jsonCmd = true;
        uint16_t index = root["manual_operation_index"].as<uint16_t>();
        uint8_t datatype = root["manual_operation_datatype"].as<uint8_t>();
        uint16_t value = root["manual_operation_value"].as<uint16_t>();
        uint8_t checked = root["manual_operation_checked"].as<uint8_t>();
        D_LOG("index: %d dt: %d value: %d checked: %d", index, datatype, value, checked);
        setSetting4030(index, datatype, value, checked, false);
      }
      if (!jsonCmd)
      {
        ithoSetSpeed(s_payload, MQTTAPI);
      }
    }
    else
    {
      if (api_cmd_allowed(s_payload))
        ithoExecCommand(s_payload, MQTTAPI);
      else
        D_LOG("Invalid MQTT API command");
    }
  }
  if (strcmp(topic, systemConfig.mqtt_domoticzout_topic) == 0)
  {
    JsonDocument root;
    DeserializationError error = deserializeJson(root, s_payload);
    if (!error)
    {
      if (!root["idx"].isNull())
      {
        uint16_t idx = root["idx"].as<uint16_t>();
        if (idx == systemConfig.mqtt_idx)
        {
          if (!root["svalue1"].isNull())
          {
            uint16_t invalue = root["svalue1"].as<uint16_t>();
            float value = invalue * 2.55;
            ithoSetSpeed((uint16_t)value, MQTTAPI);
          }
        }
      }
    }
  }
}

void updateState(uint16_t newState)
{

  // systemConfig.itho_fallback = newState; FIXME: no idea why this is in here

  if (mqttClient.connected())
  {
    char buffer[512]{};

    if (systemConfig.mqtt_domoticz_active)
    {
      int nvalue = 1;
      float state = 0.0;
      if (newState > 0.5)
      {
        state = (newState / 2.55) + 0.5;
      }
      else
      {
        state = 0.0;
        nvalue = 0;
      }
      newState = uint16_t(state);
      char buf[10]{};
      snprintf(buf, sizeof(buf), "%d", newState);

      JsonDocument root;
      root["command"] = "switchlight";
      root["idx"] = systemConfig.mqtt_idx;
      root["nvalue"] = nvalue;
      root["switchcmd"] = "Set Level";
      root["level"] = buf;
      serializeJson(root, buffer);
      mqttClient.publish(systemConfig.mqtt_domoticzin_topic, buffer, true);
    }

    snprintf(buffer, sizeof(buffer), "%d", newState);
    const std::string topic = std::string(systemConfig.mqtt_base_topic) + "/state";
    mqttClient.publish(topic.c_str(), buffer, true);
  }
}

void mqttHomeAssistantDiscovery()
{
  if (!systemConfig.mqtt_active || !mqttClient.connected() || !systemConfig.mqtt_ha_active)
    return;
  sendHomeAssistantDiscovery = false;
  I_LOG("HA DISCOVERY: Start publishing MQTT Home Assistant Discovery...");

  HADiscoveryFan();

  if (!SHT3x_original && !SHT3x_alternative && !itho_internal_hum_temp)
    return;
  HADiscoveryHumidity();
  HADiscoveryTemperature();
}

void HADiscoveryFan()
{

  /*

    HRU 300 EXAMPLE
        - fan:
          name: "Itho HRU 300 "
          device:
            identifiers: "mv"
            name: "Itho Box"
            manufacturer: "Itho Daalderop"
            model: "HRU 300"
            # ip adress of the wifi add-on
            configuration_url: "http://nrg-itho-fc00.local"
          unique_id: "Itho_hru_Fan"
          state_topic: "itho/lwt"
          state_value_template: "{% if value == 'online' %}ON{% else %}OFF{% endif %}"
    */
  char ihtostatustopic[140]{};
  char cmdtopic[140]{};
  char lwttopic[140]{};
  char statetopic[140]{};
  char modestatetopic[140]{};
  snprintf(ihtostatustopic, sizeof(ihtostatustopic), "%s%s", systemConfig.mqtt_base_topic, "/ithostatus");
  snprintf(cmdtopic, sizeof(cmdtopic), "%s%s", systemConfig.mqtt_base_topic, "/cmd");
  snprintf(lwttopic, sizeof(lwttopic), "%s%s", systemConfig.mqtt_base_topic, "/lwt");
  snprintf(statetopic, sizeof(statetopic), "%s%s", systemConfig.mqtt_base_topic, "/state");
  snprintf(modestatetopic, sizeof(modestatetopic), "%s%s", systemConfig.mqtt_base_topic, "/modestate");

  JsonDocument doc;
  JsonObject root = doc.to<JsonObject>(); // Fill the object
  char s[300]{};

  addHADevInfo(root);
  root["avty_t"] = static_cast<const char *>(lwttopic);
  snprintf(s, sizeof(s), "%s_fan", hostName());
  root["uniq_id"] = s; // unique_id

  root["name"] = "fan";
  root["stat_t"] = static_cast<const char *>(statetopic);                   // state_topic
  root["stat_val_tpl"] = "{% if value == '0' %}OFF{% else %}ON{% endif %}"; // state_value_template
  root["json_attr_t"] = static_cast<const char *>(ihtostatustopic);         // json_attributes_topic
  // snprintf(s, sizeof(s), "%s/not_used/but_needed_for_HA", static_cast<const char *>(cmdtopic));
  // root["cmd_t"] = s;
  root["cmd_t"] = static_cast<const char *>(cmdtopic);        // command_topic
  root["pct_cmd_t"] = static_cast<const char *>(cmdtopic);    // percentage_command_topic
  root["pct_stat_t"] = static_cast<const char *>(statetopic); // percentage_state_topic
  // root["pct_val_tpl"] = "{% if {{ ((value | int) / 2.55) | round }} == '0' %}OFF{% else %}{{ ((value | int) / 2.55) | round }}{% endif %} ";

  JsonArray modes = root["pr_modes"].to<JsonArray>(); // preset_modes
  modes.add("Low");
  modes.add("Medium");
  modes.add("High");
  modes.add("Auto");
  modes.add("AutoNight");
  modes.add("Timer 10min");
  modes.add("Timer 20min");
  modes.add("Timer 30min");
  root["pr_mode_cmd_t"] = static_cast<const char *>(cmdtopic);         // preset_mode_command_topic
  root["pr_mode_stat_t"] = static_cast<const char *>(ihtostatustopic); // preset_mode_state_topic

  std::string actualSpeedLabel;

  const uint8_t deviceID = currentIthoDeviceID();
  // const uint8_t version = currentItho_fwversion();
  const uint8_t deviceGroup = currentIthoDeviceGroup();

  char pr_mode_val_tpl[400]{};
  char pct_cmd_tpl[300]{};
  char pct_val_tpl[100]{};
  int pr_mode_val_tpl_ver = 0;

  if (deviceGroup == 0x07 && deviceID == 0x01) // HRU250-300
  {
    actualSpeedLabel = getStatusLabel(10, ithoDeviceptr);      //-> {"Absolute speed of the fan (%)", "absolute-speed-of-the-fan_perc"}, of hru250_300.h
    root["pr_mode_cmd_tpl"] = "{\"rfremotecmd\":\"{{value.lower()}}\"}"; // preset_mode_command_template

    strncpy(pct_cmd_tpl, "{%% if value > 90 %%}{\"rfremotecmd\":\"high\"}{%% elif value > 40 %%}{\"rfremotecmd\":\"medium\"}{%% elif value > 20 %%}{\"rfremotecmd\":\"low\"}{%% else %%}{\"rfremotecmd\":\"auto\"}{%% endif %%}", sizeof(pct_cmd_tpl));
    
    snprintf(pct_val_tpl, sizeof(pct_val_tpl), "{{ value_json['%s'] | int }}", actualSpeedLabel.c_str());
    root["pl_off"] = "{\"rfremotecmd\":\"auto\"}"; // payload_off
    pr_mode_val_tpl_ver = 1;
  }
  else if (deviceGroup == 0x00 && deviceID == 0x03) // HRU350
  {
    actualSpeedLabel = getStatusLabel(0, ithoDeviceptr);      //-> {"Requested fanspeed (%)", "requested-fanspeed_perc"}, of hru350.h
    root["pr_mode_cmd_tpl"] = "{\"vremotecmd\":\"{{value.lower()}}\"}"; // preset_mode_command_template
    strncpy(pct_cmd_tpl, "{%% if value > 90 %%}{\"vremotecmd\":\"high\"}{%% elif value > 40 %%}{\"vremotecmd\":\"medium\"}{%% elif value > 20 %%}{\"vremotecmd\":\"low\"}{%% else %%}{\"vremotecmd\":\"auto\"}{%% endif %%}", sizeof(pct_cmd_tpl));

    snprintf(pct_val_tpl, sizeof(pct_val_tpl), "{{ value_json['%s'] | int }}", actualSpeedLabel.c_str());
    root["pl_off"] = "{\"vremotecmd\":\"auto\"}"; // payload_off
    pr_mode_val_tpl_ver = 1;
  }
  else if (deviceGroup == 0x00 && deviceID == 0x0D) // WPU
  {
    // pr_mode_val_tpl_ver = ?
  }
  else if (deviceGroup == 0x00 && (deviceID == 0x0F || deviceID == 0x30)) // Autotemp
  {
    // pr_mode_val_tpl_ver = ?
  }
  else if (deviceGroup == 0x00 && deviceID == 0x0B) // DemandFlow
  {
    // pr_mode_val_tpl_ver = ?
  }
  else if (deviceGroup == 0x00 && (deviceID == 0x1D || deviceID == 0x14 || deviceID == 0x1B)) // assume CVE and HRU200 / or PWM2I2C is off
  {

    if (deviceID == 0x1D) // hru200
    {
      actualSpeedLabel = getStatusLabel(0, ithoDeviceptr); //-> {"Ventilation setpoint (%)", "ventilation-setpoint_perc"}, of hru200.h
    }
    else if (deviceID == 0x14) // cve 0x14
    {
      actualSpeedLabel = getStatusLabel(0, ithoDeviceptr); //-> {"Ventilation level (%)", "ventilation-level_perc"}, of cve14.h
    }
    else if (deviceID == 0x1B) // cve 0x1B
    {
      actualSpeedLabel = getStatusLabel(0, ithoDeviceptr); //-> {"Ventilation setpoint (%)", "ventilation-setpoint_perc"}, of cve1b.h
    }
    if (systemConfig.itho_pwm2i2c != 1)
    {
      pr_mode_val_tpl_ver = 1;
    snprintf(pct_val_tpl, sizeof(pct_val_tpl), "{{ value_json['%s'] | int }}", actualSpeedLabel.c_str());
      strncpy(pct_cmd_tpl, "{% if value > 90 %}{\"vremotecmd\":\"high\"}{% elif value > 40 %}{ \"vremotecmd\":\"medium\"}{% elif value > 20 %}{\"vremotecmd\":\"low\"}{% else %}{\"vremotecmd\":\"auto\"}{% endif %}", sizeof(pct_cmd_tpl));
      root["pr_mode_cmd_tpl"] = "{\"vremotecmd\":\"{{value.lower()}}\"}"; // preset_mode_command_template
      root["pl_off"] = "{\"vremotecmd\":\"auto\"}"; // payload_off
    }
    else
    {
      strncpy(pct_val_tpl, "{{ (value | int / 2.55) | round | int }}", sizeof(pct_val_tpl));
      strncpy(pct_cmd_tpl, "{{ (value | int * 2.55) | round | int }}", sizeof(pct_cmd_tpl));
      root["pl_off"] = "0"; // payload_off
    }
  }

  if (pr_mode_val_tpl_ver == 0)
  {
    root["pr_mode_cmd_tpl"] = "{%- if value == 'Timer 10min' %}{{'timer1'}}{%- elif value == 'Timer 20min' %}{{'timer2'}}{%- elif value == 'Timer 30min' %}{{'timer3'}}{%- else %}{{value.lower()}}{%- endif -%}";
    //snprintf(pr_mode_val_tpl, sizeof(pr_mode_val_tpl), "{%%- set speed = value_json['%s'] | int %%}{%%- if speed > 219 %%}high{%%- elif speed > 119 %%}medium{%%- elif speed > 19 %%}low{%%- else %%}auto{%%- endif -%%}", actualSpeedLabel.c_str());
    snprintf(pr_mode_val_tpl, sizeof(pr_mode_val_tpl), "{%%- set speed = value_json['%s'] | int %%}{%%- if speed > 90 %%}high{%%- elif speed > 35 %%}medium{%%- elif speed > 10 %%}low{%%- else %%}auto{%%- endif -%%}", actualSpeedLabel.c_str());

    //strncpy(pr_mode_val_tpl, "{%- if value == 'Low' %}{{'low'}}{%- elif value == 'Medium' %}{{'medium'}}{%- elif value == 'High' %}{{'high'}}{%- elif value == 'Auto' %}{{'auto'}}{%- elif value == 'AutoNight' %}{{'autonight'}}{%- elif value == 'Timer 10min' %}{{'timer1'}}{%- elif value == 'Timer 20min' %}{{'timer2'}}{%- elif value == 'Timer 30min' %}{{'timer3'}}{%- endif -%}", sizeof(pr_mode_val_tpl));
  }
  else if (pr_mode_val_tpl_ver == 1)
  {
    snprintf(pr_mode_val_tpl, sizeof(pr_mode_val_tpl), "{%%- set speed = value_json['%s'] | int %%}{%%- if speed > 90 %%}high{%%- elif speed > 35 %%}medium{%%- elif speed > 10 %%}low{%%- else %%}auto{%%- endif -%%}", actualSpeedLabel.c_str());
  }

  root["pct_cmd_tpl"] = pct_cmd_tpl;         // percentage_command_template
  root["pr_mode_val_tpl"] = pr_mode_val_tpl; // preset_mode_value_template
  root["pct_val_tpl"] = pct_val_tpl;         // percentage_value_template

  snprintf(s, sizeof(s), "%s/fan/%s/config", systemConfig.mqtt_ha_topic, hostName());

  sendHADiscovery(root, s);
}

void HADiscoveryTemperature()
{
  char cmdtopic[140]{};
  char lwttopic[140]{};
  char ihtostatustopic[140]{};
  snprintf(cmdtopic, sizeof(cmdtopic), "%s%s", systemConfig.mqtt_base_topic, "/cmd");
  snprintf(lwttopic, sizeof(lwttopic), "%s%s", systemConfig.mqtt_base_topic, "/lwt");
  snprintf(ihtostatustopic, sizeof(ihtostatustopic), "%s%s", systemConfig.mqtt_base_topic, "/ithostatus");

  JsonDocument doc;
  JsonObject root = doc.to<JsonObject>(); // Fill the object
  char s[300]{};

  addHADevInfo(root);
  root["avty_t"] = static_cast<const char *>(lwttopic);
  root["dev_cla"] = "temperature";
  snprintf(s, sizeof(s), "%s_temperature", hostName());
  root["uniq_id"] = s;
  root["name"] = "temperature";
  root["stat_t"] = static_cast<const char *>(ihtostatustopic);
  root["stat_cla"] = "measurement";
  root["val_tpl"] = "{{ value_json.temp }}";
  root["unit_of_meas"] = "°C";

  snprintf(s, sizeof(s), "%s/sensor/%s/temp/config", systemConfig.mqtt_ha_topic, hostName());

  sendHADiscovery(root, s);
}

void HADiscoveryHumidity()
{
  char lwttopic[140]{};
  char ihtostatustopic[140]{};
  snprintf(lwttopic, sizeof(lwttopic), "%s%s", systemConfig.mqtt_base_topic, "/lwt");
  snprintf(ihtostatustopic, sizeof(ihtostatustopic), "%s%s", systemConfig.mqtt_base_topic, "/ithostatus");

  JsonDocument doc;
  JsonObject root = doc.to<JsonObject>(); // Fill the object
  char s[300]{};

  addHADevInfo(root);
  root["avty_t"] = static_cast<const char *>(lwttopic);
  root["dev_cla"] = "humidity";
  snprintf(s, sizeof(s), "%s_humidity", hostName());
  root["uniq_id"] = s;
  root["name"] = "humidity";
  root["stat_t"] = static_cast<const char *>(ihtostatustopic);
  root["stat_cla"] = "measurement";
  root["val_tpl"] = "{{ value_json.hum }}";
  root["unit_of_meas"] = "%";

  snprintf(s, sizeof(s), "%s/sensor/%s/hum/config", static_cast<const char *>(systemConfig.mqtt_ha_topic), hostName());

  sendHADiscovery(root, s);
}

void addHADevInfo(JsonObject obj)
{
  char cu[50]{};
  snprintf(cu, sizeof(cu), "http://%s.local", hostName());
  JsonObject dev = obj["dev"].to<JsonObject>();
  dev["ids"] = hostName();             // identifiers
  dev["mf"] = "Arjen Hiemstra";        // manufacturer
  dev["mdl"] = "Wifi add-on for Itho"; // model
  dev["name"] = hostName();            // name
  dev["hw"] = hw_revision;             // hw_version
  dev["sw"] = FWVERSION;               // sw_version
  dev["cu"] = cu;                      // configuration_url
}

void sendHADiscovery(JsonObject obj, const char *topic)
{
  size_t payloadSize = measureJson(obj);
  // max header + topic + content. Copied logic from PubSubClien::publish(), PubSubClient.cpp:482
  size_t packetSize = MQTT_MAX_HEADER_SIZE + 2 + strlen(topic) + payloadSize;

  if (mqttClient.getBufferSize() < packetSize)
  {
    E_LOG("MQTT: buffer too small, resizing... (HA discovery)");
    mqttClient.setBufferSize(packetSize);
  }

  if (mqttClient.beginPublish(topic, payloadSize, true))
  {
    serializeJson(obj, mqttClient);
    if (!mqttClient.endPublish())
      E_LOG("MQTT: Failed to send payload (HA discovery)");
  }
  else
  {
    E_LOG("MQTT: Failed to start building message (HA discovery)");
  }

  // reset buffer
  mqttClient.setBufferSize(MQTT_BUFFER_SIZE);
}

bool setupMQTTClient()
{
  char cmdtopic[140]{};
  char lwttopic[140]{};
  snprintf(cmdtopic, sizeof(cmdtopic), "%s%s", systemConfig.mqtt_base_topic, "/cmd");
  snprintf(lwttopic, sizeof(lwttopic), "%s%s", systemConfig.mqtt_base_topic, "/lwt");

  if (systemConfig.mqtt_active)
  {
    if (strcmp(systemConfig.mqtt_serverName, "") != 0)
    {
      int connectResult;

      mqttClient.setServer(systemConfig.mqtt_serverName, systemConfig.mqtt_port);
      mqttClient.setCallback(mqttCallback);
      mqttClient.setBufferSize(MQTT_BUFFER_SIZE);

      if (strcmp(systemConfig.mqtt_username, "") == 0)
      {
        connectResult = mqttClient.connect(hostName(), lwttopic, 0, true, "offline");
      }
      else
      {
        connectResult = mqttClient.connect(hostName(), systemConfig.mqtt_username, systemConfig.mqtt_password, lwttopic, 0, true, "offline");
      }

      if (!connectResult)
      {
        return false;
      }

      if (mqttClient.connected())
      {
        mqttClient.subscribe(cmdtopic);
        if (systemConfig.mqtt_domoticz_active)
        {
          mqttClient.subscribe(systemConfig.mqtt_domoticzout_topic);
        }
        mqttClient.publish(lwttopic, "online", true);

        return true;
      }
    }
  }
  else
  {
    mqttClient.publish(lwttopic, "offline", true); // set to offline in case of graceful shutdown
    mqttClient.disconnect();
  }
  return false;
}

bool reconnect()
{
  setupMQTTClient();
  return mqttClient.connected();
}

bool api_cmd_allowed(const char *cmd)
{
  const char *apicmds[] = {"low", "medium", "auto", "high", "timer1", "timer2", "timer3", "away", "cook30", "cook60", "autonight", "motion_on", "motion_off", "join", "leave", "clearqueue"};

  for (uint8_t i = 0; i < sizeof(apicmds) / sizeof(apicmds[0]); i++)
  {
    if (strcmp(cmd, apicmds[i]) == 0)
      return true;
  }
  return false;
}
