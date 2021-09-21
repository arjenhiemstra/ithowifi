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

    if (updateIthoMQTT) {
      updateIthoMQTT = false;
      updateState(ithoCurrentVal);
    }
#if defined(ENABLE_SHT30_SENSOR_SUPPORT)
    if (temp_hum_updated) {
      temp_hum_updated = false;
      if (mqttClient.connected()) {
        char buffer[512];

        if (systemConfig.mqtt_domoticz_active) {
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
          char buffer[1024];
          StaticJsonDocument<1024> root;
          root["temp"] = ithoTemp;
          root["hum"] = ithoHum;


          auto b = 611.21 * pow(2.7183, ((18.678 - ithoTemp / 234.5) * ithoTemp) / (257.14 + ithoTemp));
          auto ppmw = b / (101325 - b) * ithoHum / 100 * 0.62145 * 1000000;
          root["ppmw"] = ppmw;

          root["itho2401len"] = itho2401len;

          for (const auto& ithoStat : ithoStatus) {
            if (ithoStat.type == ithoDeviceStatus::is_byte) {
              root[ithoStat.name.c_str()] = ithoStat.value.byteval;
            }
            else if (ithoStat.type == ithoDeviceStatus::is_uint) {
              root[ithoStat.name.c_str()] = ithoStat.value.uintval;
            }
            else if (ithoStat.type == ithoDeviceStatus::is_int) {
              root[ithoStat.name.c_str()] = ithoStat.value.intval;
            }
            else if (ithoStat.type == ithoDeviceStatus::is_float) {
              root[ithoStat.name.c_str()] = ithoStat.value.floatval;
            }
            else {
              root["error"] = 0;
            }
          }
          serializeJson(root, buffer);
          mqttClient.publish(systemConfig.mqtt_sensor_topic, buffer, true);
        }
      }
    }
    if (updateMQTTihtoStatus) {
      updateMQTTihtoStatus = false;
      if (!ithoStatus.empty()) {
        for (const auto& ithoStat : ithoStatus) {
          char val[128];
          char s[160];
          sprintf(s, "%s/%s" , (const char*)systemConfig.mqtt_ithostatus_topic, ithoStat.name.c_str());

          if (ithoStat.type == ithoDeviceStatus::is_byte) {
            sprintf(val, "%d", ithoStat.value.byteval);
          }
          else if (ithoStat.type == ithoDeviceStatus::is_uint) {
            sprintf(val, "%u", ithoStat.value.uintval);
          }
          else if (ithoStat.type == ithoDeviceStatus::is_int) {
            sprintf(val, "%d", ithoStat.value.intval);
          }
          else if (ithoStat.type == ithoDeviceStatus::is_float) {
            sprintf(val, "%.2f", ithoStat.value.floatval);
          }
          else {
            strcpy(val, "value error");
          }
          mqttClient.publish(s, val, true);
        }
        char val[32];
        char s[160];
        sprintf(val, "%d", itho2401len);
        sprintf(s, "%s/%s" , (const char*)systemConfig.mqtt_ithostatus_topic, "itho2401len");
        mqttClient.publish(s, val, true);
      }
      if (!ithoMeasurements.empty()) {
        for (const auto& ithoMeaserment : ithoMeasurements) {

          char val[128];
          char s[160];
          sprintf(s, "%s/%s" , (const char*)systemConfig.mqtt_ithostatus_topic, ithoMeaserment.name.c_str());
          if (ithoMeaserment.type == ithoDeviceMeasurements::is_int) {
            sprintf(val, "%d", ithoMeaserment.value.intval);
          }
          else if (ithoMeaserment.type == ithoDeviceMeasurements::is_float) {
            sprintf(val, "%.2f", ithoMeaserment.value.floatval);
          }
          else if (ithoMeaserment.type == ithoDeviceMeasurements::is_string) {
            sprintf(val, "%s", ithoMeaserment.value.valStatus);
          }
          else {
            strcpy(val, "value error");
          }

          mqttClient.publish(s, val, true);

        }
      }
    }

#endif
    mqttClient.loop();
  }
  else {
    if (dontReconnectMQTT) return;
    if (millis() - lastMQTTReconnectAttempt > 5000) {

      lastMQTTReconnectAttempt = millis();
      // Attempt to reconnect
      if (reconnect()) {
        lastMQTTReconnectAttempt = 0;
      }
    }
  }
}
