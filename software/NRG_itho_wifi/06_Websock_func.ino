


void jsonWsSend(String rootName, boolean nested) {

  if (rootName == "wifisettings") {
    DynamicJsonDocument root(1024);
    //JsonObject root = jsonBuffer.createObject();
    JsonObject nested = root.createNestedObject("wifisettings");
    nested[F("ssid")] = wifiConfig.ssid;
    nested[F("passwd")] = wifiConfig.passwd;
    nested[F("dhcp")] = wifiConfig.dhcp;
    nested[F("renew")] = wifiConfig.renew;
    if (nested[F("dhcp")] == "off") {
      nested[F("ip")] = wifiConfig.ip;
      nested[F("subnet")] = wifiConfig.subnet;
      nested[F("gateway")] = wifiConfig.gateway;
      nested[F("dns1")] = wifiConfig.dns1;
      nested[F("dns2")] = wifiConfig.dns2;       
    }
    else {          
      nested[F("ip")] = WiFi.localIP().toString();
      nested[F("subnet")] = WiFi.subnetMask().toString();
      nested[F("gateway")] = WiFi.gatewayIP().toString();
      nested[F("dns1")] = WiFi.dnsIP().toString();
      nested[F("dns2")] = WiFi.dnsIP(1).toString();
    }
    nested[F("port")] = wifiConfig.port;   
    size_t len = measureJson(root);
    AsyncWebSocketMessageBuffer * buffer = ws.makeBuffer(len); //  creates a buffer (len + 1) for you.
    if (buffer) {
        serializeJson(root, (char *)buffer->get(), len + 1);
        ws.textAll(buffer);
    } 
  } 
  else if (rootName == "mqttsettings") {
    DynamicJsonDocument root(1024);
    JsonObject nested = root.createNestedObject("mqttsettings");

    nested["mqtt_active"] = systemConfig.mqtt_active;
    nested["mqtt_serverName"] = systemConfig.mqtt_serverName;
    nested["mqtt_username"] = systemConfig.mqtt_username;
    nested["mqtt_password"] = systemConfig.mqtt_password;
    nested["mqtt_port"] = systemConfig.mqtt_port;
    nested["mqtt_version"] = systemConfig.mqtt_version;
    nested["mqtt_state_topic"] = systemConfig.mqtt_state_topic;
    nested["mqtt_cmd_topic"] = systemConfig.mqtt_cmd_topic;
    nested["version_of_program"] = systemConfig.version_of_program;
    
    size_t len = measureJson(root);
    AsyncWebSocketMessageBuffer * buffer = ws.makeBuffer(len); //  creates a buffer (len + 1) for you.
    if (buffer) {
        serializeJson(root, (char *)buffer->get(), len + 1);
        ws.textAll(buffer);
    }     
  }    
  

}


// Convert & Transfer Arduino elements to JSON elements
void jsonWifiscanresult(int id, String ssid, String sigval, int sec) {
        DynamicJsonDocument root(128);
        //JsonObject root = jsonBuffer.createObject();
        JsonObject wifiscanresult = root.createNestedObject("wifiscanresult");
        wifiscanresult[F("id")] = id;
        wifiscanresult[F("ssid")] = ssid;
        wifiscanresult[F("sigval")] = sigval;
        wifiscanresult[F("sec")] = sec;
        size_t len = measureJson(root);
        AsyncWebSocketMessageBuffer * buffer = ws.makeBuffer(len); //  creates a buffer (len + 1) for you.
        if (buffer) {
            serializeJson(root, (char *)buffer->get(), len + 1);
            ws.textAll(buffer);
        }
}
// Convert & Transfer Arduino elements to JSON elements
void jsonMessageBox(String message1, String message2) {
        DynamicJsonDocument root(128);
        //JsonObject root = jsonBuffer.createObject();
        JsonObject messagebox = root.createNestedObject("messagebox");
        messagebox[F("message1")] = message1;
        messagebox[F("message2")] = message2;  
        size_t len = measureJson(root);
        AsyncWebSocketMessageBuffer * buffer = ws.makeBuffer(len); //  creates a buffer (len + 1) for you.
        if (buffer) {
            serializeJson(root, (char *)buffer->get(), len + 1);
            ws.textAll(buffer);
        }
}
// Convert & Transfer Arduino elements to JSON elements
void jsonSystemstat() {
        DynamicJsonDocument root(256);
        //JsonObject root = jsonBuffer.createObject();
        JsonObject systemstat = root.createNestedObject("systemstat");

          systemstat[F("freemem")] = String(memHigh);

          systemstat[F("memlow")] = String(memLow);

          systemstat[F("mqqtstatus")] = MQTT_conn_state;

          systemstat[F("itho")] = itho_current_val;

          systemstat[F("i2cstat")] = i2cstat;
          
        size_t len = measureJson(root);
        AsyncWebSocketMessageBuffer * buffer = ws.makeBuffer(len); //  creates a buffer (len + 1) for you.
        if (buffer) {
            serializeJson(root, (char *)buffer->get(), len + 1);
            ws.textAll(buffer);
        }
}



void jsonWsReceive(String msg) {
  bool wifisettings = false;
  bool systemsettings = false;
  DynamicJsonDocument root(2048);
  DeserializationError error = deserializeJson(root, msg);
  if (!error) {
    //Serial.println("JSON receive parsed");
    // WIFI Settings parse
    if ((const char*)root[F("wifisettings")][F("ssid")] != nullptr) {
      wifisettings = true;
      strlcpy(wifiConfig.ssid, root[F("wifisettings")][F("ssid")], sizeof(wifiConfig.ssid));
    }
    if ((const char*)root[F("wifisettings")][F("password")] != nullptr) {
      wifisettings = true;
      strlcpy(wifiConfig.passwd, root[F("wifisettings")][F("password")], sizeof(wifiConfig.passwd));
    }
    if ((const char*)root[F("wifisettings")][F("dhcp")] != nullptr) {
      wifisettings = true;
      strlcpy(wifiConfig.dhcp, root[F("wifisettings")][F("dhcp")], sizeof(wifiConfig.dhcp));
    }
    if ((const char*)root[F("wifisettings")][F("renew")] != nullptr) {
      wifisettings = true;
      wifiConfig.renew = root[F("wifisettings")][F("renew")];
    }
    if ((const char*)root[F("wifisettings")][F("dhcp")] != nullptr) {
      wifisettings = true;
      strlcpy(wifiConfig.dhcp, root[F("wifisettings")][F("dhcp")], sizeof(wifiConfig.dhcp));
    }
    if ((const char*)root[F("wifisettings")][F("ip")] != nullptr) {
      wifisettings = true;
      strlcpy(wifiConfig.ip, root[F("wifisettings")][F("ip")], sizeof(wifiConfig.ip));
    }
    if ((const char*)root[F("wifisettings")][F("subnet")] != nullptr) {
      wifisettings = true;
      strlcpy(wifiConfig.subnet, root[F("wifisettings")][F("subnet")], sizeof(wifiConfig.subnet));
    }
    if ((const char*)root[F("wifisettings")][F("gateway")] != nullptr) {
      wifisettings = true;
      strlcpy(wifiConfig.gateway, root[F("wifisettings")][F("gateway")], sizeof(wifiConfig.gateway));
    }
    if ((const char*)root[F("wifisettings")][F("dns1")] != nullptr) {
      wifisettings = true;
      strlcpy(wifiConfig.dns1, root[F("wifisettings")][F("dns1")], sizeof(wifiConfig.dns1));      
    }
    if ((const char*)root[F("wifisettings")][F("dns2")] != nullptr) {
      wifisettings = true;
      strlcpy(wifiConfig.dns2, root[F("wifisettings")][F("dns2")], sizeof(wifiConfig.dns2));      
    }
    //MQTT Settings parse
    if ((const char*)root[F("mqttsettings")][F("mqtt_active")] != nullptr) {
      systemsettings = true;
      strlcpy(systemConfig.mqtt_active, root[F("mqttsettings")][F("mqtt_active")], sizeof(systemConfig.mqtt_active));      
    }    
    if ((const char*)root[F("mqttsettings")][F("mqtt_serverName")] != nullptr) {
      systemsettings = true;
      strlcpy(systemConfig.mqtt_serverName, root[F("mqttsettings")][F("mqtt_serverName")], sizeof(systemConfig.mqtt_serverName));      
    }    
    if ((const char*)root[F("mqttsettings")][F("mqtt_username")] != nullptr) {
      systemsettings = true;
      strlcpy(systemConfig.mqtt_username, root[F("mqttsettings")][F("mqtt_username")], sizeof(systemConfig.mqtt_username));      
    }    
    if ((const char*)root[F("mqttsettings")][F("mqtt_password")] != nullptr) {
      systemsettings = true;
      strlcpy(systemConfig.mqtt_password, root[F("mqttsettings")][F("mqtt_password")], sizeof(systemConfig.mqtt_password));      
    } 
    if ((const char*)root[F("mqttsettings")][F("mqtt_port")] != nullptr) {
      systemsettings = true;
      systemConfig.mqtt_port = root[F("mqttsettings")][F("mqtt_port")];   
    } 
    if ((const char*)root[F("mqttsettings")][F("mqtt_version")] != nullptr) {
      systemsettings = true;
      systemConfig.mqtt_version = root[F("mqttsettings")][F("mqtt_version")];   
    }    
    if ((const char*)root[F("mqttsettings")][F("mqtt_state_topic")] != nullptr) {
      systemsettings = true;
      strlcpy(systemConfig.mqtt_state_topic, root[F("mqttsettings")][F("mqtt_state_topic")], sizeof(systemConfig.mqtt_state_topic));      
    }    
    if ((const char*)root[F("mqttsettings")][F("mqtt_state_retain")] != nullptr) {
      systemsettings = true;
      strlcpy(systemConfig.mqtt_state_retain, root[F("mqttsettings")][F("mqtt_state_retain")], sizeof(systemConfig.mqtt_state_retain));      
    }    
    if ((const char*)root[F("mqttsettings")][F("mqtt_cmd_topic")] != nullptr) {
      systemsettings = true;
      strlcpy(systemConfig.mqtt_cmd_topic, root[F("mqttsettings")][F("mqtt_cmd_topic")], sizeof(systemConfig.mqtt_cmd_topic));      
    }
  } 
//save to file           
  if (wifisettings == true) {
    if (saveWifiConfig()) {
      jsonMessageBox("Wifi settings saved successful,", "reboot the device");
    }
    else {
      jsonMessageBox("Wifi settings save failed:", "Unable to write config file");
    } 
  }
  if (systemsettings == true) {
    if (saveSystemConfig()) {
      jsonMessageBox("System settings saved", "successful");
    }
    else {
      jsonMessageBox("System settings save failed:", "Unable to write config file");
    }
    mqttSetup = true;
    
  }        
}

void onWsEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len){
  if(type == WS_EVT_CONNECT){
    //Serial.printf("ws[%s][%u] connect\n", server->url(), client->id());
    client->ping();
  } else if(type == WS_EVT_DISCONNECT){
    //Serial.printf("ws[%s][%u] disconnect: %u\n", server->url(), client->id(), client->id());
  } else if(type == WS_EVT_ERROR){
    //Serial.printf("ws[%s][%u] error(%u): %s\n", server->url(), client->id(), *((uint16_t*)arg), (char*)data);
  } else if(type == WS_EVT_PONG){
    //Serial.printf("ws[%s][%u] pong[%u]: %s\n", server->url(), client->id(), len, (len)?(char*)data:"");
  } else if(type == WS_EVT_DATA){
    AwsFrameInfo * info = (AwsFrameInfo*)arg;
    String msg = "";
    if(info->final && info->index == 0 && info->len == len){
      //the whole message is in a single frame and we got all of it's data
      //Serial.printf("ws[%s][%u] %s-message[%llu]: ", server->url(), client->id(), (info->opcode == WS_TEXT)?"text":"binary", info->len);

      if(info->opcode == WS_TEXT){
        for(size_t i=0; i < info->len; i++) {
          msg += (char) data[i];
        }
      } else {
        char buff[3];
        for(size_t i=0; i < info->len; i++) {
          sprintf(buff, "%02x ", (uint8_t) data[i]);
          msg += buff ;
        }
      }
      //Serial.printf("%s\n",msg.c_str());

      if (msg.startsWith("{\"wifiscan")) {
        //Serial.println("Start wifi scan");
        runscan = true;
      }
      if (msg.startsWith("{\"sysstat")) {
        sysStatReq = true;
      }
      else if (msg.startsWith("{\"wifisettings")) {
        jsonWsReceive(msg);
      }
      else if (msg.startsWith("{\"mqttsettings")) {
        jsonWsReceive(msg);
      }              
      else if (msg.startsWith("{\"wifisetup")) {
        jsonWsSend("wifisettings", true);
      }              
      else if (msg.startsWith("{\"mqttsetup")) {
        jsonWsSend("mqttsettings", true);
        sysStatReq = true;
      }
      else if (msg.startsWith("{\"reboot")) {
        shouldReboot = true;
      }
      else if (msg.startsWith("{\"resetwificonf")) {
        if(resetWifiConfig()) {
          jsonMessageBox("Wifi settings restored,", "reboot the device");
        }
        else {
          jsonMessageBox("Wifi settings restore failed,", "please try again");
        }
      }
      else if (msg.startsWith("{\"resetsysconf")) {
        if(resetSystemConfig()) {
          jsonMessageBox("System settings restored,", "reboot the device");
        }        
        else {
          jsonMessageBox("System settings restore failed,", "please try again");
        }
      }
      else if (msg.startsWith("{\"itho")) {
        DynamicJsonDocument root(128);
        DeserializationError error = deserializeJson(root, msg);
        serializeJson(root, Serial);
        Serial.println();
        if (!error) {
          itho_new_val  = root["itho"];
          updateItho = true;
        }
      }
    }
  }
}





void sendScanDataWs()
{
    int n = WiFi.scanComplete();
    if(n == -2) {
      WiFi.scanNetworks(true);
    } 
    else if(n) {
      jsonMessageBox("Scan found " + String(n), "WiFi Networks");
      
      //sort networks
      int indices[n];
      for (int i = 0; i < n; i++) {
        indices[i] = i;
      }
      
      for (int i = 0; i < n; i++) {
        for (int j = i + 1; j < n; j++) {
          if (WiFi.RSSI(indices[j]) > WiFi.RSSI(indices[i])) {
            std::swap(indices[i], indices[j]);
          }
        }
      }      

      //send scan result to DOM
      for (int i = 0; i < n; i++) {
        
        int sec = 0;
        switch(WiFi.encryptionType(indices[i])) {
          case 2:
          case 4:
          case 5:
            sec = 2; //closed network
            break;
//          case 7:
//            sec = 1; //open network
//            break;                                 
//          case 8:
//            sec = 2;
//            break;
          default:
            sec = 0; //unknown network encryption
        }
        
        
        int32_t signalStrength = WiFi.RSSI(indices[i]);
        uint8_t signalStrengthResult = 0;

        if (signalStrength > -65) {
           signalStrengthResult = 5;
        }
        else if (signalStrength > -75) {
           signalStrengthResult = 4;
        }
        else if (signalStrength > -80) {
           signalStrengthResult = 3;
        }
        else if (signalStrength > -90) {
           signalStrengthResult = 2;
        }
        else if (signalStrength > -100) {
           signalStrengthResult = 1;
        }
        else {
           signalStrengthResult = 0;
        }
        
        jsonWifiscanresult(i, String(WiFi.SSID(indices[i])), String(signalStrengthResult), sec);
      }
      WiFi.scanDelete();
      if(WiFi.scanComplete() == -2){
        WiFi.scanNetworks(true);
      }
    }
    
}


int LastPercentotaWSupdate = 0;

void otaWSupdate(size_t prg, size_t sz) {
      int newPercent = int((prg*100)/content_len);
      if (newPercent != LastPercentotaWSupdate) {
        LastPercentotaWSupdate = newPercent;
        if (newPercent % 2 == 0) {
          DynamicJsonDocument root(256);
          JsonObject ota = root.createNestedObject("ota");
          ota[F("progress")] = String(prg);
          ota[F("tsize")] = String(content_len);
          ota[F("percent")] = String(newPercent);
          size_t len = measureJson(root);
          AsyncWebSocketMessageBuffer * buffer = ws.makeBuffer(len); //  creates a buffer (len + 1) for you.
          if (buffer) {
              serializeJson(root, (char *)buffer->get(), len + 1);
              ws.textAll(buffer);
          } 
      }  
      }
    
}  
