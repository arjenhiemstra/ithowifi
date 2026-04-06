#include "api/MqttAPI.h"
#include "tasks/task_mqtt.h"

static void mqttSendResponse(const char *status, const char *command, const char *message)
{
  char responseTopic[160]{};
  snprintf(responseTopic, sizeof(responseTopic), "%s%s", systemConfig.mqtt_base_topic, "/cmd/response");

  JsonDocument doc;
  doc["status"] = status;
  doc["command"] = command;
  if (message)
    doc["message"] = message;
  time_t now;
  time(&now);
  doc["timestamp"] = now;

  char buf[256]{};
  serializeJson(doc, buf, sizeof(buf));
  mqttClient.publish(responseTopic, buf);
}

void mqttCallback(const char *topic, const byte *payload, unsigned int length)
{

  if (topic == NULL)
    return;
  if (payload == NULL)
    return;

  if (length > 1023)
    length = 1023;

  char s_payload[length + 1];
  std::memcpy(s_payload, payload, length);
  s_payload[length] = '\0';

  bool clean_cmd_topic = false;

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
        mqttSendResponse("success", value, nullptr);
        clean_cmd_topic = true;
      }
      if (!root["vremote"].isNull() || !root["vremotecmd"].isNull())
      {
        const char *command = root["vremote"] | "";
        if (strcmp(command, "") == 0)
          command = root["vremotecmd"] | "";

        int index = -1;
        if (!root["vremotename"].isNull())
        {
          index = virtualRemotes.getRemoteIndexbyName((const char *)root["vremotename"]);
        }
        else if (!root["vremoteindex"].isNull())
        {
          index = root["vremoteindex"];
        }
        else
        {
          index = 0; // default to index 0 if neither specified
        }
        if (index >= 0)
        {
          jsonCmd = true;
          ithoI2CCommand(index, command, MQTTAPI);
          mqttSendResponse("success", "vremotecmd", command);
          clean_cmd_topic = true;
        }
        else
        {
          mqttSendResponse("fail", "vremotecmd", "invalid remote index or name");
        }
      }
      if (!root["rfremotecmd"].isNull() || !root["rfremoteindex"].isNull())
      {
        uint8_t idx = 0;
        if (!root["rfremoteindex"].isNull())
        {
          idx = root["rfremoteindex"].as<uint8_t>();
        }
        if (!root["rfremotecmd"].isNull())
        {
          jsonCmd = true;
          if (ithoExecRFCommand(idx, root["rfremotecmd"], MQTTAPI))
            mqttSendResponse("success", "rfremotecmd", root["rfremotecmd"]);
          else
            mqttSendResponse("fail", "rfremotecmd", "rf command failed");
          clean_cmd_topic = true;
        }
      }
      if (!root["rfco2"].isNull())
      {
        uint8_t idx = 0;
        if (!root["rfremoteindex"].isNull())
        {
          idx = root["rfremoteindex"].as<uint8_t>();
        }
        jsonCmd = true;
        if (ithoSendRFCO2(idx, root["rfco2"].as<uint16_t>(), MQTTAPI))
          mqttSendResponse("success", "rfco2", "co2 value sent");
        else
          mqttSendResponse("fail", "rfco2", "failed - remote must be RFT CO2 type");
        clean_cmd_topic = true;
      }
      if (!root["rfdemand"].isNull())
      {
        uint8_t idx = 0;
        if (!root["rfremoteindex"].isNull())
        {
          idx = root["rfremoteindex"].as<uint8_t>();
        }
        uint8_t zone = 0;
        if (!root["rfzone"].isNull())
        {
          zone = root["rfzone"].as<uint8_t>();
        }
        jsonCmd = true;
        if (ithoSendRFDemand(idx, root["rfdemand"].as<uint8_t>(), zone, MQTTAPI))
          mqttSendResponse("success", "rfdemand", "demand value sent");
        else
          mqttSendResponse("fail", "rfdemand", "failed - remote must be RFT CO2 or RFT RV type");
        clean_cmd_topic = true;
      }
      if (!root["percentage"].isNull() || !root["fandemand"].isNull())
      {
        jsonCmd = true;
        int demand = 0;
        if (!root["percentage"].isNull())
        {
          int pct = root["percentage"].as<int>();
          demand = (pct < 0) ? 0 : (pct > 100) ? 200 : pct * 2;
        }
        else
        {
          demand = root["fandemand"].as<int>();
          if (demand < 0) demand = 0;
          if (demand > 200) demand = 200;
        }

        if (systemConfig.itho_control_interface == 1)
        {
          int rfIdx = -1;
          for (int ri = 0; ri < remotes.getMaxRemotes(); ri++)
          {
            if (remotes.isEmptySlot(ri)) continue;
            if (remotes.getRemoteFunction(ri) == RemoteFunctions::SEND &&
                remotes.getRemoteType(ri) == RemoteTypes::RFTCO2)
            { rfIdx = ri; break; }
          }
          if (rfIdx >= 0)
          {
            ithoExecRFCommand(rfIdx, "auto", MQTTAPI);
            delay(200);
            ithoSendRFDemand(rfIdx, (uint8_t)demand, 0, MQTTAPI);
            mqttSendResponse("success", "percentage", "auto + demand sent");
          }
          else
            mqttSendResponse("fail", "percentage", "no Send+RFTCO2 remote");
        }
        else if (systemConfig.itho_pwm2i2c == 1)
        {
          int speed = (demand * 255) / 200;
          ithoSetSpeed((uint16_t)speed, MQTTAPI);
          mqttSendResponse("success", "percentage", nullptr);
        }
        else
        {
          mqttSendResponse("fail", "percentage", "no RF CO2 or PWM2I2C available");
        }
        clean_cmd_topic = true;
      }
      if (!root["speed"].isNull())
      {
        jsonCmd = true;
        if (!root["timer"].isNull())
        {
          ithoSetSpeedTimer(root["speed"].as<uint16_t>(), root["timer"].as<uint16_t>(), MQTTAPI);
          mqttSendResponse("success", "speed+timer", nullptr);
          clean_cmd_topic = true;
        }
        else
        {
          ithoSetSpeed(root["speed"].as<uint16_t>(), MQTTAPI);
          mqttSendResponse("success", "speed", nullptr);
          clean_cmd_topic = true;
        }
      }
      else if (!root["timer"].isNull())
      {
        jsonCmd = true;
        ithoSetTimer(root["timer"].as<uint16_t>(), MQTTAPI);
        mqttSendResponse("success", "timer", nullptr);
        clean_cmd_topic = true;
      }
      if (!root["clearqueue"].isNull())
      {
        jsonCmd = true;
        const char *value = root["clearqueue"] | "";
        if (strcmp(value, "true") == 0)
        {
          clearQueue = true;
          mqttSendResponse("success", "clearqueue", nullptr);
          clean_cmd_topic = true;
        }
        else
        {
          mqttSendResponse("fail", "clearqueue", "value must be \"true\"");
        }
      }
      if (!(const char *)root["outside_temp"].isNull())
      {
        jsonCmd = true;
        float outside_temp = root["outside_temp"].as<float>();
        float temporary_outside_temp = root["temporary_outside_temp"].as<float>();
        uint32_t valid_until = root["valid_until"].as<uint32_t>();
        setSettingCE30(static_cast<int16_t>(temporary_outside_temp * 100), static_cast<int16_t>(outside_temp * 100), valid_until, false);
        mqttSendResponse("success", "outside_temp", nullptr);
        clean_cmd_topic = true;
      }
      if (!(const char *)root["manual_operation_index"].isNull())
      {
        jsonCmd = true;
        uint16_t index = root["manual_operation_index"].as<uint16_t>();
        uint8_t datatype = root["manual_operation_datatype"].as<uint8_t>();
        uint16_t value = root["manual_operation_value"].as<uint16_t>();
        uint8_t checked = root["manual_operation_checked"].as<uint8_t>();
        D_LOG("API: index: %d dt: %d value: %d checked: %d", index, datatype, value, checked);
        setSetting4030(index, datatype, value, checked, false);
        mqttSendResponse("success", "manual_operation", nullptr);
        clean_cmd_topic = true;
      }
      if (!root["update_url"].isNull())
      {
        jsonCmd = true;
        const char *url = root["update_url"] | "";
        if (strncmp(url, "https://github.com/arjenhiemstra/ithowifi/", 42) == 0)
        {
          mqttSendResponse("success", "update_url", "update starting");
          clean_cmd_topic = true;
          triggerOTAUpdateFromURL(url);
        }
        else
        {
          mqttSendResponse("fail", "update_url", "invalid URL");
          clean_cmd_topic = true;
        }
      }
      else if (!root["update"].isNull())
      {
        jsonCmd = true;
        const char *value = root["update"] | "stable";
        bool beta = (strcmp(value, "beta") == 0);
        if (strlen(beta ? firmwareInfo.link_beta : firmwareInfo.link) > 0)
        {
          mqttSendResponse("success", "update", beta ? "beta update starting" : "update starting");
          clean_cmd_topic = true;
          triggerOTAUpdate(beta);
        }
        else
        {
          mqttSendResponse("fail", "update", "no firmware URL available");
          clean_cmd_topic = true;
        }
      }
      if (!jsonCmd)
      {
        ithoSetSpeed(s_payload, MQTTAPI);
        mqttSendResponse("success", "speed", s_payload);
        clean_cmd_topic = true;
      }
    }
    else
    {
      if (apiCmdAllowed(s_payload))
      {
        ithoExecCommand(s_payload, MQTTAPI);
        mqttSendResponse("success", s_payload, nullptr);
        clean_cmd_topic = true;
      }
      else
      {
        mqttSendResponse("fail", s_payload, "unknown command");
        D_LOG("API: Invalid MQTT API command");
      }
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
            clean_cmd_topic = true;
          }
        }
      }
    }
  }
  if (clean_cmd_topic)
  {
    mqttClient.publish(c_topic, "", true);
  }
}

bool apiCmdAllowed(const char *cmd)
{
  const char *apicmds[] = {"low", "medium", "auto", "high", "timer1", "timer2", "timer3", "away", "cook30", "cook60", "autonight", "motion_on", "motion_off", "join", "leave", "clearqueue"};

  for (uint8_t i = 0; i < sizeof(apicmds) / sizeof(apicmds[0]); i++)
  {
    if (strcmp(cmd, apicmds[i]) == 0)
      return true;
  }
  return false;
}
