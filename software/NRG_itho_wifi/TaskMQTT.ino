#if defined (HW_VERSION_TWO)
#include <cmath>

void startTaskMQTT() {
  xTaskMQTTHandle = xTaskCreateStaticPinnedToCore(
                      TaskMQTT,
                      "TaskMQTT",
                      STACK_SIZE_LARGE,
                      ( void * ) 1,
                      TASK_MQTT_PRIO,
                      xTaskMQTTStack,
                      &xTaskMQTTBuffer,
                      CONFIG_ARDUINO_RUNNING_CORE);
}

void TaskMQTT( void * pvParameters ) {
  configASSERT( ( uint32_t ) pvParameters == 1UL );


  mqttInit();

  startTaskWeb();

  esp_task_wdt_add(NULL);

  for (;;) {
    yield();
    esp_task_wdt_reset();

    TaskMQTTTimeout.once_ms(35000UL, []() {
      logInput("Error: Task MQTT timed out!");
    });

    execMQTTTasks();

    TaskMQTTHWmark = uxTaskGetStackHighWaterMark( NULL );
    vTaskDelay(25 / portTICK_PERIOD_MS);
  }

  //else delete task
  vTaskDelete( NULL );
}

#endif


void execMQTTTasks() {

  if (systemConfig.mqtt_updated) {
    systemConfig.mqtt_updated = false;
    setupMQTTClient();
  }
  // handle MQTT:
  if (systemConfig.mqtt_active == 0) {
    MQTT_conn_state_new = -5;
  }
  else {
    MQTT_conn_state_new = mqttClient.state();
  }
  if (MQTT_conn_state != MQTT_conn_state_new) {
    MQTT_conn_state = MQTT_conn_state_new;
    sysStatReq = true;
  }
  if (mqttClient.connected()) {
    if(sendHomeAssistantDiscovery) {
      mqttHomeAssistantDiscovery();
    }
    if (updateIthoMQTT) {
      updateIthoMQTT = false;
      updateState(ithoCurrentVal);
    }
    if (updateMQTTihtoStatus) {
      updateMQTTihtoStatus = false;

      if (mqttClient.connected()) {


        if (systemConfig.mqtt_domoticz_active) {
          char buffer[512];
          auto humstat = 0;
          // Humidity Status
          if (ithoHum < 31) {
            humstat = 2;
          }
          else if (ithoHum > 69) {
            humstat = 3;
          }
          else if (ithoHum > 34 && ithoHum < 66 && ithoTemp > 21 && ithoTemp < 27) {
            humstat = 1;
          }

          char svalue[32];
          sprintf(svalue, "%1.1f;%1.1f;%d", ithoTemp, ithoHum, humstat);

          StaticJsonDocument<512> root;
          root["svalue"] = svalue;
          root["nvalue"] = 0;
          root["idx"] = systemConfig.sensor_idx;
          serializeJson(root, buffer);
          mqttClient.publish(systemConfig.mqtt_state_topic, buffer, true);
        }
        else {
          mqttSendStatus();
          mqttSendRemotesInfo();
          mqttPublishLastcmd();
        }
      }
    }
    mqttClient.loop();
  }
  else {
    if (dontReconnectMQTT) return;
    if (millis() - lastMQTTReconnectAttempt > 5000) {

      lastMQTTReconnectAttempt = millis();
      // Attempt to reconnect
      if (reconnect()) {
        sendHomeAssistantDiscovery = true;
        lastMQTTReconnectAttempt = 0;
      }
    }
  }
}

void mqttSendStatus() {

  DynamicJsonDocument doc(6000);

  if (doc.capacity() == 0) {
    logInput("MQTT: JsonDocument memory allocation failed (itho status)");
    return;
  }

  JsonObject root = doc.to<JsonObject>();
  getIthoStatusJSON(root);

  size_t len = measureJson(root);

  if (mqttClient.getBufferSize() < len) {
    mqttClient.setBufferSize(len);
  }
  if (mqttClient.beginPublish(systemConfig.mqtt_ithostatus_topic, len, true)) {
    serializeJson(root, mqttClient);
    if (!mqttClient.endPublish()) logInput("MQTT: Failed to send payload (itho status)");
  }
  // reset buffer
  mqttClient.setBufferSize(MQTT_BUFFER_SIZE);

}
void mqttSendRemotesInfo() {
  DynamicJsonDocument doc(1000);

  if (doc.capacity() == 0) {
    logInput("MQTT: JsonDocument memory allocation failed (itho remote info)");
    return;
  }

  JsonObject root = doc.to<JsonObject>();
    
  getRemotesInfoJSON(root);

  size_t len = measureJson(root);

  if (mqttClient.getBufferSize() < len) {
    mqttClient.setBufferSize(len);
  }
  if (mqttClient.beginPublish(systemConfig.mqtt_remotesinfo_topic, len, true)) {
    serializeJson(root, mqttClient);
    if (!mqttClient.endPublish()) logInput("MQTT: Failed to send payload (itho remote info))");
  }
  // reset buffer
  mqttClient.setBufferSize(MQTT_BUFFER_SIZE);
}

void mqttPublishLastcmd() {
  DynamicJsonDocument doc(1000);

  JsonObject root = doc.to<JsonObject>();
    
  getLastCMDinfoJSON(root);

  size_t len = measureJson(root);

  if (mqttClient.getBufferSize() < len) {
    mqttClient.setBufferSize(len);
  }
  if (mqttClient.beginPublish(systemConfig.mqtt_lastcmd_topic, len, true)) {
    serializeJson(root, mqttClient);
    if (!mqttClient.endPublish()) logInput("MQTT: Failed to send payload (last cmd info))");
  }
  // reset buffer
  mqttClient.setBufferSize(MQTT_BUFFER_SIZE);
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {


  if (topic == NULL) return;
  if (payload == NULL) return;

  bool dtype = true;
  if (systemConfig.mqtt_domoticz_active) {
    dtype = false;
  }

  if (length > 1023) length = 1023;

  char s_payload[length];
  memcpy(s_payload, payload, length);
  s_payload[length] = '\0';

  if (strcmp(topic, systemConfig.mqtt_cmd_topic) == 0) {
    StaticJsonDocument<1024> root;
    DeserializationError error = deserializeJson(root, s_payload);
    if (!error) {
      bool jsonCmd = false;
      if (!(const char*)root["idx"].isNull()) {
        jsonCmd = true;
        //printf("JSON parse -- idx match\n");
        uint16_t idx = root["idx"].as<uint16_t>();
        if (idx == systemConfig.mqtt_idx) {
          if (!(const char*)root["svalue1"].isNull()) {
            uint16_t invalue = root["svalue1"].as<uint16_t>();
            double value = invalue * 2.54;
            ithoSetSpeed((uint16_t)value, MQTTAPI);
          }
        }
      }
      if (!(const char*)root["dtype"].isNull()) {
        const char* value = root["dtype"] | "";
        if (strcmp(value, "ithofan") == 0) {
          dtype = true;
        }
      }
      if (dtype) {
        /*
           standard true, unless mqtt_domoticz_active == "on"
           if mqtt_domoticz_active == "on"
              this should be set to true first by a JSON containing key:value pair "dtype":"ithofan",
              otherwise different commands might get processed due to domoticz general domoticz/out topic structure
        */
        if (!(const char*)root["command"].isNull()) {
          jsonCmd = true;
          const char* value = root["command"] | "";
          ithoExecCommand(value, MQTTAPI);
        }
        if (!(const char*)root["vremote"].isNull()) {
          jsonCmd = true;
          const char* value = root["vremote"] | "";
          ithoI2CCommand(value, MQTTAPI);
        }
        if (!(const char*)root["speed"].isNull()) {
          jsonCmd = true;
          ithoSetSpeed(root["speed"].as<uint16_t>(), MQTTAPI);
        }
        if (!(const char*)root["timer"].isNull()) {
          jsonCmd = true;
          ithoSetTimer(root["timer"].as<uint16_t>(), MQTTAPI);
        }
        if (!(const char*)root["clearqueue"].isNull()) {
          jsonCmd = true;
          const char* value = root["clearqueue"] | "";
          if (strcmp(value, "true") == 0) {
            clearQueue = true;
          }
        }
        if (!jsonCmd) {
          ithoSetSpeed(s_payload, MQTTAPI);
        }
      }

    }
    else {
      ithoExecCommand(s_payload, MQTTAPI);
    }

  }
  else {
    //topic unknown
  }
}

void updateState(uint16_t newState) {

  systemConfig.itho_fallback = newState;

  if (mqttClient.connected()) {
    char buffer[512];

    if (systemConfig.mqtt_domoticz_active) {
      int nvalue = 1;
      double state = 1.0;
      if (newState > 0) {
        state  = newState / 2.54;
      }

      newState = uint16_t(state + 0.5);
      char buf[10];
      sprintf(buf, "%d", newState);

      StaticJsonDocument<512> root;
      root["command"] = "switchlight";
      root["idx"] = systemConfig.mqtt_idx;
      root["nvalue"] = nvalue;
      root["switchcmd"] = "Set Level";
      root["level"] = buf;
      serializeJson(root, buffer);
    }
    else {
      sprintf(buffer, "%d", newState);
    }
    mqttClient.publish(systemConfig.mqtt_state_topic, buffer, true);

  }
}


void mqttHomeAssistantDiscovery()
{
  if (!systemConfig.mqtt_active || !mqttClient.connected() || !systemConfig.mqtt_ha_active) return;
  sendHomeAssistantDiscovery = false;
  logInput("HA DISCOVERY: Start publishing MQTT Home Assistant Discovery...");

  HADiscoveryFan();

  if (!SHT3x_original && !SHT3x_alternative && !itho_internal_hum_temp) return;
  HADiscoveryHumidity();
  HADiscoveryTemperature();
}


void HADiscoveryFan() {
  DynamicJsonDocument doc(2048);
  JsonObject root = doc.to<JsonObject>(); // Fill the object
  char s[160];

  addHADevInfo(root);
  root["avty_t"] = (const char*)systemConfig.mqtt_lwt_topic;
  sprintf(s, "%s_fan", hostName());
  root["uniq_id"] = s;
  root["name"] = s;
  root["stat_t"] = (const char*)systemConfig.mqtt_lwt_topic;
  root["stat_val_tpl"] = "{% if value == 'online' %}ON{% else %}OFF{% endif %}";
  root["json_attr_t"] = (const char*)systemConfig.mqtt_ithostatus_topic;
  sprintf(s, "%s/not_used/but_needed_for_HA", systemConfig.mqtt_cmd_topic);
  root["cmd_t"] = s;
  root["pct_cmd_t"] = (const char*)systemConfig.mqtt_cmd_topic;
  root["pct_cmd_tpl"] = "{{ value * 2.54 }}";
  root["pct_stat_t"] = (const char*)systemConfig.mqtt_state_topic;
  root["pct_val_tpl"] = "{{ ((value | int) / 2.54) | round}}";

  sprintf(s, "%s/fan/%s/config" , (const char*)systemConfig.mqtt_ha_topic, hostName());

  sendHADiscovery(root, s);

}

void HADiscoveryTemperature() {
  DynamicJsonDocument doc(2048);
  JsonObject root = doc.to<JsonObject>(); // Fill the object
  char s[160];

  addHADevInfo(root);
  root["avty_t"] = (const char*)systemConfig.mqtt_lwt_topic;
  root["dev_cla"] = "temperature";
  sprintf(s, "%s_temperature", hostName());
  root["uniq_id"] = s;
  root["name"] = s;
  root["stat_t"] = (const char*)systemConfig.mqtt_ithostatus_topic;
  root["val_tpl"] = "{{ value_json.temp }}";
  root["unit_of_meas"] = "°C";

  sprintf(s, "%s/sensor/%s/temp/config" , (const char*)systemConfig.mqtt_ha_topic, hostName());

  sendHADiscovery(root, s);
}

void HADiscoveryHumidity() {
  DynamicJsonDocument doc(2048);
  JsonObject root = doc.to<JsonObject>(); // Fill the object
  char s[160];

  addHADevInfo(root);
  root["avty_t"] = (const char*)systemConfig.mqtt_lwt_topic;
  root["dev_cla"] = "humidity";
  sprintf(s, "%s_humidity", hostName());
  root["uniq_id"] = s;
  root["name"] = s;
  root["stat_t"] = (const char*)systemConfig.mqtt_ithostatus_topic;
  root["val_tpl"] = "{{ value_json.hum }}";

  sprintf(s, "%s/sensor/%s/hum/config" , (const char*)systemConfig.mqtt_ha_topic, hostName());

  sendHADiscovery(root, s);
}

void addHADevInfo(JsonObject obj) {
  char s[64];
  JsonObject dev = obj.createNestedObject("dev");
  dev["identifiers"] = hostName();
  dev["manufacturer"] = "Arjen Hiemstra";
  dev["model"] = "ITHO Wifi Add-on";
  sprintf(s, "ITHO-WIFI(%s)", hostName());
  dev["name"] = s;
  sprintf(s, "HW: v%s, FW: %s", HWREVISION, FWVERSION);
  dev["sw_version"] = s;

}

void sendHADiscovery(JsonObject obj, const char* topic)
{
  size_t payloadSize = measureJson(obj);
  //max header + topic + content. Copied logic from PubSubClien::publish(), PubSubClient.cpp:482
  size_t packetSize = MQTT_MAX_HEADER_SIZE + 2 + strlen(topic) + payloadSize;

  if (mqttClient.getBufferSize() < packetSize)
  {
    logInput("MQTT: buffer too small, resizing... (HA discovery)");
    mqttClient.setBufferSize(packetSize);
  }

  if (mqttClient.beginPublish(topic, payloadSize, true))
  {
    serializeJson(obj, mqttClient);
    if (!mqttClient.endPublish()) logInput("MQTT: Failed to send payload (HA discovery)");
  }
  else
  {
    logInput("MQTT: Failed to start building message (HA discovery)");
  }

  // reset buffer
  mqttClient.setBufferSize(MQTT_BUFFER_SIZE);
}

bool setupMQTTClient() {
  int connectResult;

  if (systemConfig.mqtt_active) {

    if (strcmp(systemConfig.mqtt_serverName, "") != 0) {

      mqttClient.setServer(systemConfig.mqtt_serverName, systemConfig.mqtt_port);
      mqttClient.setCallback(mqttCallback);
      mqttClient.setBufferSize(MQTT_BUFFER_SIZE);

      if (strcmp(systemConfig.mqtt_username, "") == 0) {
        connectResult = mqttClient.connect(hostName(), systemConfig.mqtt_lwt_topic, 0, true, "offline");
      }
      else {
        connectResult = mqttClient.connect(hostName(), systemConfig.mqtt_username, systemConfig.mqtt_password, systemConfig.mqtt_lwt_topic, 0, true, "offline");
      }

      if (!connectResult) {
        return false;
      }

      if (mqttClient.connected()) {
        mqttClient.subscribe(systemConfig.mqtt_cmd_topic);
        mqttClient.publish(systemConfig.mqtt_lwt_topic, "online", true);

        return true;
      }
    }

  }
  else {
    mqttClient.publish(systemConfig.mqtt_lwt_topic, "offline", true);  //set to offline in case of graceful shutdown
    mqttClient.disconnect();
  }
  return false;

}

boolean reconnect() {
  setupMQTTClient();
  return mqttClient.connected();
}
