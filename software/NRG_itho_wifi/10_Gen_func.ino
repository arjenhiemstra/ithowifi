void getIthoStatusJSON(JsonObject root) {
  root["temp"] = ithoTemp;
  root["hum"] = ithoHum;

  auto b = 611.21 * pow(2.7183, ((18.678 - ithoTemp / 234.5) * ithoTemp) / (257.14 + ithoTemp));
  auto ppmw = b / (101325 - b) * ithoHum / 100 * 0.62145 * 1000000;
  root["ppmw"] = ppmw;
  if (!ithoInternalMeasurements.empty()) {
    for (const auto& internalMeasurement : ithoInternalMeasurements) {
      if (internalMeasurement.type == ithoDeviceMeasurements::is_int) {
        root[internalMeasurement.name.c_str()] = internalMeasurement.value.intval;
      }
      else if (internalMeasurement.type == ithoDeviceMeasurements::is_float) {
        root[internalMeasurement.name.c_str()] = internalMeasurement.value.floatval;
      }
      else {
        root["error"] = 0;
      }
    }
  }
  if (!ithoMeasurements.empty()) {
    for (const auto& ithoMeaserment : ithoMeasurements) {
      if (ithoMeaserment.type == ithoDeviceMeasurements::is_int) {
        root[ithoMeaserment.name.c_str()] = ithoMeaserment.value.intval;
      }
      else if (ithoMeaserment.type == ithoDeviceMeasurements::is_float) {
        root[ithoMeaserment.name.c_str()] = ithoMeaserment.value.floatval;
      }
      else if (ithoMeaserment.type == ithoDeviceMeasurements::is_string) {
        root[ithoMeaserment.name.c_str()] = ithoMeaserment.value.stringval;
      }
    }
  }
  if (!ithoStatus.empty()) {
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
      else if (ithoStat.type == ithoDeviceStatus::is_string) {
        root[ithoStat.name.c_str()] = ithoStat.value.stringval;
      }
    }

  }
}

void getRemotesInfoJSON(JsonObject root) {

  remotes.getCapabilities(root);

}

bool ithoExecCommand(const char* command, cmdOrigin origin) {
  D_LOG("EXEC COMMAND\n");
  if (systemConfig.itho_vremoteapi) {
    return ithoI2CCommand(command, origin);
  }
  else {
    if (strcmp(command, "low") == 0) {
      ithoSetSpeed(systemConfig.itho_low, origin);
    }
    else if (strcmp(command, "medium") == 0) {
      ithoSetSpeed(systemConfig.itho_medium, origin);
    }
    else if (strcmp(command, "high") == 0) {
      ithoSetSpeed(systemConfig.itho_high, origin);
    }
    else if (strcmp(command, "timer1") == 0) {
      ithoSetTimer(systemConfig.itho_timer1, origin);
    }
    else if (strcmp(command, "timer2") == 0) {
      ithoSetTimer(systemConfig.itho_timer2, origin);
    }
    else if (strcmp(command, "timer3") == 0) {
      ithoSetTimer(systemConfig.itho_timer3, origin);
    }
    else if (strcmp(command, "clearqueue") == 0) {
      clearQueue = true;
    }
    else {
      return false;
    }    
  }

  return true;

}


bool ithoI2CCommand(const char* command, cmdOrigin origin) {

  D_LOG("EXEC VREMOTE COMMAND\n");
#if defined (HW_VERSION_TWO)
  if (xSemaphoreTake(mutexI2Ctask, (TickType_t) 500 / portTICK_PERIOD_MS) == pdTRUE) {
  }
  else {
    return false;
  }
#endif

  if (strcmp(command, "low") == 0) {
    buttonValue = 1;
    sendI2CButton = true;
  }
  else if (strcmp(command, "medium") == 0) {
    buttonValue = 2;
    sendI2CButton = true;
  }
  else if (strcmp(command, "high") == 0) {
    buttonValue = 3;
    sendI2CButton = true;
  }
  else if (strcmp(command, "timer1") == 0) {
    timerValue = systemConfig.itho_timer1;
    sendI2CTimer = true;
  }
  else if (strcmp(command, "timer2") == 0) {
    timerValue = systemConfig.itho_timer2;
    sendI2CTimer = true;
  }
  else if (strcmp(command, "timer3") == 0) {
    timerValue = systemConfig.itho_timer3;
    sendI2CTimer = true;
  }
  else if (strcmp(command, "join") == 0) {
    sendI2CJoin = true;
  }
  else if (strcmp(command, "leave") == 0) {
    sendI2CLeave = true;
  }
  else if (strcmp(command, "type") == 0) {
    sendI2CDevicetype = true;
  }
  else if (strcmp(command, "status") == 0) {
    sendI2CStatus = true;
  }
  else if (strcmp(command, "statusformat") == 0) {
    sendI2CStatusFormat = true;
  }
  else if (strcmp(command, "31DA") == 0) {
    send31DA = true;
  }
  else if (strcmp(command, "31D9") == 0) {
    send31D9 = true;
  }
  else {
#if defined (HW_VERSION_TWO)
    xSemaphoreGive(mutexI2Ctask);
#endif
    return false;
  }

  logLastCommand(command, origin);
  return true;

}

bool ithoSetSpeed(const char* speed, cmdOrigin origin) {
  uint16_t val = strtoul(speed, NULL, 10);
  return ithoSetSpeed(val, origin);
}

bool ithoSetSpeed(uint16_t speed, cmdOrigin origin) {
  D_LOG("SET SPEED\n");
  if (speed < 255) {
    nextIthoVal = speed;
    nextIthoTimer = 0;
    updateItho();
  }
  else {
    return false;
  }

  char buf[32] {};
  sprintf(buf, "speed:%d", speed);
  logLastCommand(buf, origin);
  return true;
}

bool ithoSetTimer(const char* timer, cmdOrigin origin) {
  uint16_t t = strtoul(timer, NULL, 10);
  return ithoSetTimer(t, origin);
}

bool ithoSetTimer(uint16_t timer, cmdOrigin origin) {
  D_LOG("SET TIMER\n");
  if (timer > 0 && timer < 65535) {
    nextIthoTimer = timer;
    nextIthoVal = systemConfig.itho_high;
    updateItho();
  }
  else {
    return false;
  }

  char buf[32] {};
  sprintf(buf, "timer:%d", timer);
  logLastCommand(buf, origin);
  return true;

}

bool ithoSetSpeedTimer(const char* speed, const char* timer, cmdOrigin origin) {
  uint16_t speedval = strtoul(speed, NULL, 10);
  uint16_t timerval = strtoul(speed, NULL, 10);
  return ithoSetSpeedTimer(speedval, timerval, origin);
}

bool ithoSetSpeedTimer(uint16_t speed, uint16_t timer, cmdOrigin origin) {
  D_LOG("SET SPEED AND TIMER\n");
  if (speed < 255) {
    nextIthoVal = speed;
    nextIthoTimer = timer;
    updateItho();
  }
  else {
    return false;
  }

  char buf[32] {};
  sprintf(buf, "speed:%d,timer:%d", speed, timer);
  logLastCommand(buf, origin);
  return true;
}

void logLastCommand(const char* command, cmdOrigin origin) {

  if (origin != REMOTE) {
    const char* source;
    auto it = cmdOriginMap.find(origin);
    if (it != cmdOriginMap.end()) source = it->second;
    else source = cmdOriginMap.rbegin()->second;
    logLastCommand(command, source);
  }
  else {
    logLastCommand(command, remotes.lastRemoteName);
  }

}

void logLastCommand(const char* command, const char* source) {

  lastCmd.source = source;
  strcpy(lastCmd.command, command);

  if (time(nullptr)) {
    time_t now;
    time(&now);
    lastCmd.timestamp = now;

  } else
  {
    lastCmd.timestamp = (time_t)millis();
  }

}

void getLastCMDinfoJSON(JsonObject root) {

  root["source"] = lastCmd.source;
  root["command"] = lastCmd.command;
  root["timestamp"] = lastCmd.timestamp;

}

void updateItho() {
  if (systemConfig.itho_rf_support) {
    IthoCMD.once_ms(150, add2queue);
  }
  else {
    add2queue();
  }
}

void add2queue() {
  ithoQueue.add2queue(nextIthoVal, nextIthoTimer, systemConfig.nonQ_cmd_clearsQ);
}



// Update itho Value
bool writeIthoVal(uint16_t value) {

  if (value > 254) {
    value = 254;
  }

  if (ithoCurrentVal != value) {

#if defined (HW_VERSION_TWO)
    if (xSemaphoreTake(mutexI2Ctask, (TickType_t) 500 / portTICK_PERIOD_MS) == pdTRUE) {
    }
    else {
      return false;
    }
#endif

    uint16_t valTemp = ithoCurrentVal;
    ithoCurrentVal = value;

    int timeout = 0;
    while (digitalRead(SCLPIN) == LOW && timeout < 1000) { //'check' if other master is active
      yield();
      delay(1);
      timeout++;
    }
    if (timeout != 1000) {

      if (systemConfig.itho_forcemedium) {
        buttonValue = 2;
        buttonResult = false;
        sendI2CButton = true;
        auto timeoutmillis = millis() + 400;
        while (!buttonResult && millis() < timeoutmillis) {
          //wait for result
        }
      }
      updateIthoMQTT = true;

      uint8_t command[] = {0x00, 0x60, 0xC0, 0x20, 0x01, 0x02, 0xFF, 0x00, 0xFF};

      uint8_t b = (uint8_t) value;

      command[6] = b;
      //command[8] = 0 - (67 + b);
      command[sizeof(command) - 1] = checksum(command, sizeof(command) - 1);

#if defined (HW_VERSION_ONE)
      Wire.beginTransmission(byte(0x00));
      for (uint8_t i = 1; i < sizeof(command); i++) {
        Wire.write(command[i]);
      }
      Wire.endTransmission(true);
#elif defined (HW_VERSION_TWO)
      i2c_sendBytes(command, sizeof(command));
#endif

    }
    else {
      logInput("Warning: I2C timeout");
      ithoCurrentVal = valTemp;
      ithoQueue.add2queue(valTemp, 0, systemConfig.nonQ_cmd_clearsQ);
    }
#if defined (HW_VERSION_TWO)
    xSemaphoreGive(mutexI2Ctask);
#endif
    return true;
  }
  return false;
}


void printTimestamp(Print * _logOutput) {
#if defined (HW_VERSION_ONE)
  if (time(nullptr)) {
    time_t now;
    struct tm * timeinfo;
    time(&now);
    timeinfo = localtime(&now);

    char timeStringBuff[50];  // 50 chars should be enough
    strftime(timeStringBuff, sizeof(timeStringBuff), "%F %T ", timeinfo);
    _logOutput->print(timeStringBuff);
  } else
#elif defined (HW_VERSION_TWO)
  struct tm timeinfo;
  if (getLocalTime(&timeinfo, 0)) {
    char timeStringBuff[50];  // 50 chars should be enough
    strftime(timeStringBuff, sizeof(timeStringBuff), "%F %T ", &timeinfo);
    _logOutput->print(timeStringBuff);
  } else
#endif
  {
    char c[32];
    sprintf(c, "%10lu ", millis());
    _logOutput->print(c);
  }
}

void printNewline(Print * _logOutput) {
  _logOutput->print("\n");
}

void logInput(const char * inputString) {
#if defined (HW_VERSION_TWO)
  yield();
  if (xSemaphoreTake(mutexLogTask, (TickType_t) 500 / portTICK_PERIOD_MS) == pdTRUE) {
#endif

    filePrint.open();

    Log.notice(inputString);

    filePrint.close();

#if defined (ENABLE_SERIAL)
    D_LOG("%s\n", inputString);
#endif

#if defined (HW_VERSION_TWO)
    xSemaphoreGive(mutexLogTask);
  }
#endif


}


void logWifiInfo() {

  char wifiBuff[128];

  logInput("WiFi: connection successful");


  logInput("WiFi info:");

  const char* const modes[] = { "NULL", "STA", "AP", "STA+AP" };
  //const char* const phymodes[] = { "", "B", "G", "N" };

#if defined (HW_VERSION_ONE)
  sprintf(wifiBuff, "Mode:%s", modes[wifi_get_opmode()]);
  logInput(wifiBuff);
  strcpy(wifiBuff, "");

  sprintf(wifiBuff, "PHY mode:%s", phymodes[(int) wifi_get_phy_mode()]);
  logInput(wifiBuff);
  strcpy(wifiBuff, "");

  sprintf(wifiBuff, "Channel:%d", wifi_get_channel());
  logInput(wifiBuff);
  strcpy(wifiBuff, "");

  sprintf(wifiBuff, "AP id:%d", wifi_station_get_current_ap_id());
  logInput(wifiBuff);
  strcpy(wifiBuff, "");

  sprintf(wifiBuff, "Status:%d", wifi_station_get_connect_status());
  logInput(wifiBuff);
  strcpy(wifiBuff, "");

  sprintf(wifiBuff, "Auto connect:%d", wifi_station_get_auto_connect());
  logInput(wifiBuff);
  strcpy(wifiBuff, "");

  struct station_config conf;
  wifi_station_get_config(&conf);

  char ssid[33]; //ssid can be up to 32chars, => plus null term
  memcpy(ssid, conf.ssid, sizeof(conf.ssid));
  ssid[32] = 0; //nullterm in case of 32 char ssid

  sprintf(wifiBuff, "SSID (%d):%s", strlen(ssid), ssid);
  logInput(wifiBuff);
  strcpy(wifiBuff, "");

#elif defined (HW_VERSION_TWO)

  sprintf(wifiBuff, "Mode:%s", modes[WiFi.getMode()]);
  logInput(wifiBuff);
  strcpy(wifiBuff, "");

  sprintf(wifiBuff, "Status:%d", WiFi.status());
  logInput(wifiBuff);
  strcpy(wifiBuff, "");
#endif

  IPAddress ip = WiFi.localIP();
  sprintf(wifiBuff, "IP:%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
  logInput(wifiBuff);
  strcpy(wifiBuff, "");

}
