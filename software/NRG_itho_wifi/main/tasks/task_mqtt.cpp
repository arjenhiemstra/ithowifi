#include "tasks/task_mqtt.h"
#include <StreamUtils.h>
#include "api/MqttAPI.h"

#define TASK_MQTT_PRIO 5

// globals
TaskHandle_t xTaskMQTTHandle = NULL;
uint32_t TaskMQTTHWmark = 0;
PubSubClient mqttClient(networkManager.standardClient);

// locals
StaticTask_t xTaskMQTTBuffer;
StackType_t xTaskMQTTStack[STACK_SIZE_MEDIUM];
bool sendHomeAssistantDiscovery = false;
bool updateIthoMQTT = false;

Ticker TaskMQTTTimeout;

// #include <cmath>

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

    TaskMQTTTimeout.once_ms(TASK_MQTT_TIMEOUT_MS, []()
                            { W_LOG("SYS: warning - Task MQTT timed out!"); });

    execMQTTTasks();

    TaskMQTTHWmark = uxTaskGetStackHighWaterMark(NULL);
    vTaskDelay(25 / portTICK_PERIOD_MS);
  }

  TaskHandle_t xTempTask = xTaskMQTTHandle;
  xTaskMQTTHandle = NULL;
  vTaskDelete(xTempTask);
}

void MQTTSendBuffered(JsonDocument doc, const char *topic)
{
  mqttClient.beginPublish(topic, measureJson(doc), true);
  BufferingPrint bufferedClient(mqttClient, 32);
  serializeJson(doc, bufferedClient);
  bufferedClient.flush();
  mqttClient.endPublish();
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
    if (sendHomeAssistantDiscovery && HADiscConfigLoaded)
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
        mqttPublishDeviceInfo();
      }
    }
    mqttClient.loop();
  }
  else
  {
    if (dontReconnectMQTT)
      return;
    if (millis() - lastMQTTReconnectAttempt > MQTT_RECONNECT_INTERVAL_MS)
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
      E_LOG("API: mqtt connection failed, System config: %d", systemConfig.mqtt_active);
    }
    else
    {
      N_LOG("API: mqtt connected, System config: %d", systemConfig.mqtt_active);
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
      E_LOG("API: Failed to send payload (itho status)");
  }
  if (doc.overflowed())
  {
    E_LOG("API: JsonDocument overflowed (itho status)");
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

  if (doc.overflowed())
    E_LOG("API: mqttSendRemotesInfo overflowed!");

  MQTTSendBuffered(doc, remotesinfotopic);
}

void mqttPublishLastcmd()
{
  char lastcmdtopic[140]{};
  snprintf(lastcmdtopic, sizeof(lastcmdtopic), "%s%s", systemConfig.mqtt_base_topic, "/lastcmd");
  JsonDocument doc;

  JsonObject root = doc.to<JsonObject>();

  getLastCMDinfoJSON(root);

  if (doc.overflowed())
    E_LOG("API: mqttSendRemotesInfo overflowed!");

  MQTTSendBuffered(doc, lastcmdtopic);
}

void mqttPublishDeviceInfo()
{
  char deviceinfotopic[140]{};
  snprintf(deviceinfotopic, sizeof(deviceinfotopic), "%s%s", systemConfig.mqtt_base_topic, "/deviceinfo");
  JsonDocument doc;

  JsonObject root = doc.to<JsonObject>();

  getDeviceInfoJSON(root);

  if (doc.overflowed())
    E_LOG("API: mqttSendRemotesInfo overflowed!");

  MQTTSendBuffered(doc, deviceinfotopic);
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
  if (!ithoStatusReady())
    return;

  sendHomeAssistantDiscovery = false;
  N_LOG("HAD: publishing HA Discovery");

  JsonDocument configDoc;
  JsonObject configObj = configDoc.to<JsonObject>();

  haDiscConfig.get(configObj);

  JsonDocument outputDoc;
  JsonObject outputObj = outputDoc.to<JsonObject>();

  generateHADiscoveryJson(configObj, outputObj);

  // config doc is no longer needed, manually clear memory before next step
  configDoc.clear();

  char devicetopic[160]{};

  snprintf(devicetopic, sizeof(devicetopic), "%s%s%s%s", systemConfig.mqtt_ha_topic, "/device/", hostName(), "/config");

  if (outputDoc.overflowed())
    E_LOG("HAD: generateHADiscoveryJson overflowed!");

  MQTTSendBuffered(outputDoc, devicetopic);
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
