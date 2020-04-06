



bool initFileSystem() {

  //Serial.println("Mounting FS...");

  #if ESP8266

    if (!SPIFFS.begin())
    {
      //Serial.println("SPIFFS failed, needs formatting");
      handleFormat();
      delay(500);
      ESP.restart();
    }
    else
    {
      FSInfo fs_info;
      if (!SPIFFS.info(fs_info))
      {
        //Serial.println("fs_info failed");
        return false;
      }
      else
      {
        FSTotal = fs_info.totalBytes;
        FSUsed = fs_info.usedBytes;
        //Serial.println("File System loaded");
        //Serial.print("Disk size: ");Serial.println(FSTotal);
        //Serial.print("Disk used: ");Serial.println(FSUsed);
      }
    }

  
  #endif
  #if ESP32
  SPIFFS.begin(true);
  #endif

  return true;
}


void handleFormat()
{
  //Serial.println("Format SPIFFS");
  if (SPIFFS.format())
  {
    if (!SPIFFS.begin())
    {
      //Serial.println("Format SPIFFS failed");
    }
  }
  else
  {
    //Serial.println("Format SPIFFS failed");
  }
  if (!SPIFFS.begin())
  {
    //Serial.println("SPIFFS failed, needs formatting");
  }
  else
  {
    //Serial.println("SPIFFS mounted");
  }
}

char * EspHostname() {
  // Do a little work to get a unique-ish name. Append the
  // last two bytes of the MAC (HEX'd):
  uint8_t mac[6];
  WiFi.softAPmacAddress(mac);
  
  String macID = String(mac[6 - 2], HEX) +
                 String(mac[6 - 1], HEX);
  macID.toUpperCase();
  String AP_NameString = espName + macID;

  char *AP_NameChar = (char *) malloc(sizeof(char) * (AP_NameString.length() + 1));
  //char AP_NameChar[AP_NameString.length() + 1];
  //memset(AP_NameChar, 0, AP_NameString.length() + 1);

  for (int i=0; i<AP_NameString.length(); i++)
    AP_NameChar[i] = AP_NameString.charAt(i);

  return AP_NameChar;
}

void setupWiFiAP()
{

  /* Soft AP network parameters */
  IPAddress apIP(192, 168, 4, 1);
  IPAddress netMsk(255, 255, 255, 0);
  
  WiFi.persistent(false);
  // disconnect sta, start ap
  WiFi.disconnect(); //  this alone is not enough to stop the autoconnecter
  WiFi.mode(WIFI_AP);
  WiFi.persistent(true);

  WiFi.softAPConfig(apIP, apIP, netMsk);
  WiFi.softAP(EspHostname(), WiFiAPPSK);

  delay(500);

  /* Setup the DNS server redirecting all the domains to the apIP */  
  dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
  dnsServer.start(53, "*", apIP);

  wifiModeAP = true;
  digitalWrite(WIFILED, LOW);  
}


bool connectWiFiSTA()
{
  wifiModeAP = false;
  //Serial.println("Connecting wifi network");
  WiFi.disconnect();
  delay(100);
  
  WiFi.mode(WIFI_STA);
  
#if defined(ESP8266)
  WiFi.hostname(EspHostname());
  WiFi.begin(wifiConfig.ssid, wifiConfig.passwd);
#else
  WiFi.begin(wifiConfig.ssid, wifiConfig.passwd);
  WiFi.setHostname(EspHostname());
#endif
  
  int i = 0;
  while (WiFi.status() != WL_CONNECTED && i < 31) {
    delay(1000);
    //Serial.print(".");
    ++i;
  }
  if (WiFi.status() != WL_CONNECTED && i >= 30) {
    //delay(1000);
    //Serial.println("");
    //Serial.println("Couldn't connect to network :( ");

    return false;

  }
  else {
 
  }
  digitalWrite(WIFILED, LOW);
  return true;
    
}

bool setupMQTTClient() {
  int connectResult;

  if (strcmp(systemConfig.mqtt_active, "on") == 0) {
    
    if (systemConfig.mqtt_serverName != "") {
      mqttClient.setServer(systemConfig.mqtt_serverName, systemConfig.mqtt_port);
      mqttClient.setCallback(mqttCallback);
      
      if (systemConfig.mqtt_username == "") {
        connectResult = mqttClient.connect(EspHostname());
      }
      else {
        connectResult = mqttClient.connect(EspHostname(), systemConfig.mqtt_username, systemConfig.mqtt_password);
      }
      
      if (!connectResult) {
        return false;
      }
      
      if (mqttClient.connected()) {
        if (mqttClient.subscribe(systemConfig.mqtt_state_topic)) {
          //publish succes
        }
        else {
          //publish failed
        }
        if (mqttClient.subscribe(systemConfig.mqtt_cmd_topic)) {
          //subscribed succes
        }
        else {
          //subscribed failed
        }
        return true;
      }
    }
    
  }
  else {
    mqttClient.disconnect();
    return false;
  }

}

boolean reconnect() {
  setupMQTTClient();
  return mqttClient.connected();
} 
