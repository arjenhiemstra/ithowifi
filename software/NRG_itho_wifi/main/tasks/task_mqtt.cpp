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

          StaticJsonDocument<512> root;
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

  DynamicJsonDocument doc(6000);

  if (doc.capacity() == 0)
  {
    E_LOG("MQTT: JsonDocument memory allocation failed (itho status)");
    return;
  }

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
  // reset buffer
  mqttClient.setBufferSize(MQTT_BUFFER_SIZE);
}
void mqttSendRemotesInfo()
{
  char remotesinfotopic[140]{};
  snprintf(remotesinfotopic, sizeof(remotesinfotopic), "%s%s", systemConfig.mqtt_base_topic, "/remotesinfo");

  DynamicJsonDocument doc(1000);

  if (doc.capacity() == 0)
  {
    E_LOG("MQTT: JsonDocument memory allocation failed (itho remote info)");
    return;
  }

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
  // reset buffer
  mqttClient.setBufferSize(MQTT_BUFFER_SIZE);
}

void mqttPublishLastcmd()
{
  char lastcmdtopic[140]{};
  snprintf(lastcmdtopic, sizeof(lastcmdtopic), "%s%s", systemConfig.mqtt_base_topic, "/lastcmd");
  DynamicJsonDocument doc(1000);

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
  DynamicJsonDocument doc(4000);

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
    StaticJsonDocument<1024> root;
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
      //       double value = invalue * 2.54;
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
          if (index != -1)
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
      if (!jsonCmd)
      {
        ithoSetSpeed(s_payload, MQTTAPI);
      }
    }
    else
    {
      ithoExecCommand(s_payload, MQTTAPI);
    }
  }
  if (strcmp(topic, systemConfig.mqtt_domoticzout_topic) == 0)
  {
    StaticJsonDocument<1024> root;
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
            double value = invalue * 2.55;
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
      double state = 0.0;
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

      StaticJsonDocument<512> root;
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

add:

  'pr_mode_cmd_t':       'preset_mode_command_topic',
  'pr_mode_cmd_tpl':     'preset_mode_command_template',
  'pr_mode_stat_t':      'preset_mode_state_topic',
  'pr_mode_val_tpl':     'preset_mode_value_template',
  'pr_modes':            'preset_modes',

  preset_mode_command_topic: itho/cmd
      preset_mode_command_template: >
        {%- if value == "Low" %}{{"low"}}
        {%- elif value == "Medium" %}{{"medium"}}
        {%- elif value == "High" %}{{"high"}}
        {%- elif value == "Timer 10min" %}{{"timer1"}}
        {%- elif value == "Timer 20min" %}{{"timer2"}}
        {%- elif value == "Timer 30min" %}{{"timer3"}}
        {%- elif value == "Max" %}{{(100*255/100)|round(0)}}
        {%- endif -%}
      preset_modes:
        - "Low"
        - "Medium"
        - "High"
        - "Timer 10min"
        - "Timer 20min"
        - "Timer 30min"
        - "Max"

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

  DynamicJsonDocument doc(2048);
  JsonObject root = doc.to<JsonObject>(); // Fill the object
  char s[300]{};

  addHADevInfo(root);
  root["avty_t"] = static_cast<const char *>(lwttopic);
  snprintf(s, sizeof(s), "%s_fan", hostName());
  root["uniq_id"] = s;
  root["name"] = hostName();
  // root["stat_t"] = static_cast<const char *>(statetopic);
  // root["stat_val_tpl"] = "{% if value == '0' %}OFF{% else %}ON{% endif %}";
  root["json_attr_t"] = static_cast<const char *>(ihtostatustopic);
  snprintf(s, sizeof(s), "%s/not_used/but_needed_for_HA", static_cast<const char *>(cmdtopic));
  root["cmd_t"] = s;
  root["pct_cmd_t"] = static_cast<const char *>(cmdtopic);
  root["pct_cmd_tpl"] = "{{ (value * 2.55) | round | int }}";
  root["pct_stat_t"] = static_cast<const char *>(statetopic);
  root["pct_val_tpl"] = "{{ (value / 2.55) | round | int }}";
  //root["pct_val_tpl"] = "{% if {{ ((value | int) / 2.55) | round }} == '0' %}OFF{% else %}{{ ((value | int) / 2.55) | round }}{% endif %} ";

  JsonArray modes = root.createNestedArray("pr_modes");
  modes.add("Low");
  modes.add("Medium");
  modes.add("High");
  modes.add("Timer 10min");
  modes.add("Timer 20min");
  modes.add("Timer 30min");
  root["pr_mode_cmd_t"] = cmdtopic;
  //root["pr_mode_stat_t"] = modestatetopic;
  root["pr_mode_cmd_tpl"] = "{%- if value == 'Low' %}{{'low'}}{%- elif value == 'Medium' %}{{'medium'}}{%- elif value == 'High' %}{{'high'}}{%- elif value == 'Timer 10min' %}{{'timer1'}}{%- elif value == 'Timer 20min' %}{{'timer2'}}{%- elif value == 'Timer 30min' %}{{'timer3'}}{%- endif -%}";

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

  DynamicJsonDocument doc(2048);
  JsonObject root = doc.to<JsonObject>(); // Fill the object
  char s[300]{};

  addHADevInfo(root);
  root["avty_t"] = static_cast<const char *>(lwttopic);
  root["dev_cla"] = "temperature";
  snprintf(s, sizeof(s), "%s_temperature", hostName());
  root["uniq_id"] = s;
  root["name"] = s;
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

  DynamicJsonDocument doc(2048);
  JsonObject root = doc.to<JsonObject>(); // Fill the object
  char s[300]{};

  addHADevInfo(root);
  root["avty_t"] = static_cast<const char *>(lwttopic);
  root["dev_cla"] = "humidity";
  snprintf(s, sizeof(s), "%s_humidity", hostName());
  root["uniq_id"] = s;
  root["name"] = s;
  root["stat_t"] = static_cast<const char *>(ihtostatustopic);
  root["stat_cla"] = "measurement";
  root["val_tpl"] = "{{ value_json.hum }}";
  root["unit_of_meas"] = "%";

  snprintf(s, sizeof(s), "%s/sensor/%s/hum/config", static_cast<const char *>(systemConfig.mqtt_ha_topic), hostName());

  sendHADiscovery(root, s);
}

void addHADevInfo(JsonObject obj)
{
  char s[64]{};
  JsonObject dev = obj.createNestedObject("dev");
  dev["identifiers"] = hostName();
  dev["manufacturer"] = "Arjen Hiemstra";
  dev["model"] = "Wifi add-on for Itho";
  snprintf(s, sizeof(s), "%s", hostName());
  dev["name"] = s;
  dev["hw_version"] = hw_revision;
  dev["sw_version"] = FWVERSION;
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

boolean reconnect()
{
  setupMQTTClient();
  return mqttClient.connected();
}
