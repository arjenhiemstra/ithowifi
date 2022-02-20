void getIthoStatusJSON(JsonObject root) {
  if (SHT3x_original || SHT3x_alternative || itho_internal_hum_temp) {
    root["temp"] = static_cast<int>(ithoTemp * 10 + 0.5) / 10.0;
    root["hum"] = static_cast<int>(ithoHum * 10 + 0.5) / 10.0;

    auto b = 611.21 * pow(2.7183, ((18.678 - ithoTemp / 234.5) * ithoTemp) / (257.14 + ithoTemp));
    auto ppmw = b / (101325 - b) * ithoHum / 100 * 0.62145 * 1000000;
    root["ppmw"] = static_cast<int>(ppmw+0.5);
  }

  if (!ithoInternalMeasurements.empty()) {
    for (const auto& internalMeasurement : ithoInternalMeasurements) {
      if (internalMeasurement.type == ithoDeviceMeasurements::is_int) {
        root[internalMeasurement.name] = internalMeasurement.value.intval;
      }
      else if (internalMeasurement.type == ithoDeviceMeasurements::is_float) {
        root[internalMeasurement.name] = internalMeasurement.value.floatval;
      }
      else {
        root["error"] = 0;
      }
    }
  }
  if (!ithoMeasurements.empty()) {
    for (const auto& ithoMeaserment : ithoMeasurements) {
      if (ithoMeaserment.type == ithoDeviceMeasurements::is_int) {
        root[ithoMeaserment.name] = ithoMeaserment.value.intval;
      }
      else if (ithoMeaserment.type == ithoDeviceMeasurements::is_float) {
        root[ithoMeaserment.name] = ithoMeaserment.value.floatval;
      }
      else if (ithoMeaserment.type == ithoDeviceMeasurements::is_string) {
        root[ithoMeaserment.name] = ithoMeaserment.value.stringval;
      }
    }
  }
  if (!ithoStatus.empty()) {
    for (const auto& ithoStat : ithoStatus) {
      if (ithoStat.type == ithoDeviceStatus::is_byte) {
        root[ithoStat.name] = ithoStat.value.byteval;
      }
      else if (ithoStat.type == ithoDeviceStatus::is_uint) {
        root[ithoStat.name] = ithoStat.value.uintval;
      }
      else if (ithoStat.type == ithoDeviceStatus::is_int) {
        root[ithoStat.name] = ithoStat.value.intval;
      }
      else if (ithoStat.type == ithoDeviceStatus::is_float) {
        root[ithoStat.name] = ithoStat.value.floatval;
      }
      else if (ithoStat.type == ithoDeviceStatus::is_string) {
        root[ithoStat.name] = ithoStat.value.stringval;
      }
    }

  }
}

void getRemotesInfoJSON(JsonObject root) {

  remotes.getCapabilities(root);

}

void getIthoSettingsBackupJSON(JsonObject root) {

  if (ithoSettingsArray != nullptr) {
    for (int i = 0; i < ithoSettingsLength; i++) {
      char buf[12];
      itoa(i, buf, 10);

      if (ithoSettingsArray[i].type == ithoSettings::is_int8) {
        int8_t val;
        std::memcpy(&val, &ithoSettingsArray[i].value, sizeof(val));
        root[buf] = val;
      }
      else if (ithoSettingsArray[i].type == ithoSettings::is_int16) {
        int16_t val;
        std::memcpy(&val, &ithoSettingsArray[i].value, sizeof(val));
        root[buf] = val;
      }
      else if (ithoSettingsArray[i].type == ithoSettings::is_int32) {
        root[buf] = ithoSettingsArray[i].value;
      }
      else {
        uint32_t val;
        std::memcpy(&val, &ithoSettingsArray[i].value, sizeof(val));
        root[buf] = val;
      }


    }
  }

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
    else if (strcmp(command, "cook30") == 0) {
      ithoSetTimer(30, origin);
    }
    else if (strcmp(command, "cook60") == 0) {
      ithoSetTimer(60, origin);
    }
    else if (strcmp(command, "auto") == 0) {
      ithoSetSpeed(systemConfig.itho_medium, origin);
    }
    else if (strcmp(command, "autonight") == 0) {
      ithoSetSpeed(systemConfig.itho_medium, origin);
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

bool ithoI2CButtonCommand(uint8_t remoteIndex, const char* command) {
  D_LOG("EXEC VREMOTE BUTTON COMMAND\n");

  if (xSemaphoreTake(mutexI2Ctask, (TickType_t) 500 / portTICK_PERIOD_MS) == pdTRUE) {
  }
  else {
    return false;
  }
  if (strcmp(command, "low") == 0) {
    sendRemoteCmd(remoteIndex, IthoLow, virtualRemotes);
  }
  
  else if (strcmp(command, "medium") == 0) {
    sendRemoteCmd(remoteIndex, IthoMedium, virtualRemotes);
  }
  else if (strcmp(command, "high") == 0) {
    sendRemoteCmd(remoteIndex, IthoHigh, virtualRemotes);
  }
  else if (strcmp(command, "timer1") == 0) {
    sendRemoteCmd(remoteIndex, IthoTimer1, virtualRemotes);
  }
  else if (strcmp(command, "timer2") == 0) {
    sendRemoteCmd(remoteIndex, IthoTimer2, virtualRemotes);
  }
  else if (strcmp(command, "timer3") == 0) {
    sendRemoteCmd(remoteIndex, IthoTimer3, virtualRemotes);
  }
  else if (strcmp(command, "cook30") == 0) {
    sendRemoteCmd(remoteIndex, IthoCook30, virtualRemotes);
  }
  else if (strcmp(command, "cook60") == 0) {
    sendRemoteCmd(remoteIndex, IthoCook60, virtualRemotes);
  }    
  else if (strcmp(command, "auto") == 0) {
    sendRemoteCmd(remoteIndex, IthoAuto, virtualRemotes);
  }
  else if (strcmp(command, "autonight") == 0) {
    sendRemoteCmd(remoteIndex, IthoAutoNight, virtualRemotes);
  }    
  else if (strcmp(command, "join") == 0) {
    sendRemoteCmd(remoteIndex, IthoJoin, virtualRemotes);
  }
  else if (strcmp(command, "leave") == 0) {
    sendRemoteCmd(remoteIndex, IthoLeave, virtualRemotes);
  }
  else {

    xSemaphoreGive(mutexI2Ctask);

    return false;
  }
  char origin[15] {};
  sprintf(origin, "vremote-%d", remoteIndex);
  
  logLastCommand(command, origin);
  
  xSemaphoreGive(mutexI2Ctask);
  
  return true;
  
}
bool ithoI2CCommand(const char* command, cmdOrigin origin) {

  D_LOG("EXEC VREMOTE COMMAND\n");

  if (xSemaphoreTake(mutexI2Ctask, (TickType_t) 500 / portTICK_PERIOD_MS) == pdTRUE) {
  }
  else {
    return false;
  }


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
  else if (strcmp(command, "cook30") == 0) {
    //tbi
  }
  else if (strcmp(command, "cook60") == 0) {
    //tbi
  }    
  else if (strcmp(command, "auto") == 0) {
    //tbi
  }
  else if (strcmp(command, "autonight") == 0) {
    //tbi
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
  else if (strcmp(command, "10D0") == 0) {
    send10D0 = true;
  }  
  else {

    xSemaphoreGive(mutexI2Ctask);

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
  uint16_t timerval = strtoul(timer, NULL, 10);
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

    if (xSemaphoreTake(mutexI2Ctask, (TickType_t) 500 / portTICK_PERIOD_MS) == pdTRUE) {
    }
    else {
      return false;
    }

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

      i2c_sendBytes(command, sizeof(command));

    }
    else {
      logInput("Warning: I2C timeout");
      ithoCurrentVal = valTemp;
      ithoQueue.add2queue(valTemp, 0, systemConfig.nonQ_cmd_clearsQ);
    }

    xSemaphoreGive(mutexI2Ctask);

    return true;
  }
  return false;
}





void logWifiInfo() {

  char wifiBuff[128];

  logInput("WiFi: connection successful");


  logInput("WiFi info:");

  const char* const modes[] = { "NULL", "STA", "AP", "STA+AP" };
  //const char* const phymodes[] = { "", "B", "G", "N" };

  sprintf(wifiBuff, "Mode:%s", modes[WiFi.getMode()]);
  logInput(wifiBuff);
  strcpy(wifiBuff, "");

  sprintf(wifiBuff, "Status:%d", WiFi.status());
  logInput(wifiBuff);
  strcpy(wifiBuff, "");

  IPAddress ip = WiFi.localIP();
  sprintf(wifiBuff, "IP:%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
  logInput(wifiBuff);
  strcpy(wifiBuff, "");

}
