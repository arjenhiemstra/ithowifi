
const char html_mainpage[] PROGMEM = R"=====(
<!doctype html>
<html lang="en">
<head>
    <meta charset="utf-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <meta name="description" content="A layout example with a side menu that hides on mobile, just like the Pure website.">
    <title>Itho WiFi controller &ndash; System setup</title>
    <script src="/js/zepto.min.js"></script>
    <script src="/js/controls.js"></script>
    <link rel="stylesheet" href="pure-min.css">
</head>
<body>
<div id="layout">
<div id="message_box"></div>
    <!-- Menu toggle -->
    <a href="#menu" id="menuLink" class="menu-link">
        <!-- Hamburger icon -->
        <span></span>
    </a>
    <div id="menu">
        <div class="pure-menu">
            <a class="pure-menu-heading" id="headingindex" href="index">Itho WiFi controller</a>
            <ul class="pure-menu-list">
                <li class="pure-menu-item"><a href="wifisetup" class="pure-menu-link">Wifi setup</a></li>
                <li class="pure-menu-item"><a href="system" class="pure-menu-link">System settings</a></li>
                <li class="pure-menu-item"><a href="itho" class="pure-menu-link">Itho settings</a></li>
                <li class="pure-menu-item"><a href="mqtt" class="pure-menu-link">MQTT</a></li>
                <li class="pure-menu-item"><a href="api" class="pure-menu-link">API</a></li>
                <li class="pure-menu-item"><a href="help" class="pure-menu-link">Help</a></li>
                <li class="pure-menu-item"><a href="update" class="pure-menu-link">Update</a></li>
                <li class="pure-menu-item"><a href="reset" class="pure-menu-link">Reset</a></li>
                <li class="pure-menu-item"><a href="debug" class="pure-menu-link">Debug</a></li>
            </ul>
        <div id="memory_box"></div>
        </div>
    </div>
    <div id="main">
    </div>
</div>
<div id="loader">
<div style="width: 100px;position: absolute;top: calc(50% - 7px);left: calc(50% - 50px);text-align: center;">Connecting...</div>
<div id="spinner"></div>
</div>
</body>
<script src="/js/general.js"></script>
</html>
)=====";


void handleAPI(AsyncWebServerRequest *request) {
  bool parseOK = false;
  nextIthoTimer = 0;
  
  int params = request->params();
  
  if (strcmp(systemConfig.syssec_api, "on") == 0) {
    bool username = false;
    bool password = false;
    
    for(int i=0;i<params;i++){
      AsyncWebParameter* p = request->getParam(i);
      if(strcmp(p->name().c_str(), "username") == 0) {
        if (strcmp(p->value().c_str(), systemConfig.sys_username) == 0 ) {
          username = true;
        }
      }
      else if(strcmp(p->name().c_str(), "password") == 0) {
        if (strcmp(p->value().c_str(), systemConfig.sys_password) == 0 ) {
          password = true;
        }
      }
    }
    if (!username || !password) {
      request->send(200, "text/html", "AUTHENTICATION FAILED");
      return;
    }  
  }
  
  for(int i=0;i<params;i++){
    char logBuff[LOG_BUF_SIZE] = "";
    AsyncWebParameter* p = request->getParam(i);
    
    if(strcmp(p->name().c_str(), "get") == 0) {
      AsyncWebParameter* p = request->getParam("get");
      if (strcmp(p->value().c_str(), "currentspeed") == 0 ) {
        char ithoval[5];
        sprintf(ithoval, "%d", ithoCurrentVal);
        request->send(200, "text/html", ithoval);
        return;
      }
      else if (strcmp(p->value().c_str(), "queue") == 0 ) {
        AsyncResponseStream *response = request->beginResponseStream("text/html");        
        DynamicJsonDocument root(1000);
        JsonObject obj = root.to<JsonObject>(); // Fill the object
        ithoQueue.get(obj);
        obj["ithoSpeed"] = ithoQueue.ithoSpeed;
        obj["ithoOldSpeed"] = ithoQueue.ithoOldSpeed;
        obj["fallBackSpeed"] = ithoQueue.fallBackSpeed;
        serializeJson(root, *response);
        request->send(response);
        
        return;
      }      
    }
    else if(strcmp(p->name().c_str(), "debug") == 0) {
      if (strcmp(p->value().c_str(), "format") == 0 ) {
        if (SPIFFS.format()) {
          strcpy(logBuff, "Filesystem format = success");
          dontSaveConfig = true;
        }
        else {
          strcpy(logBuff, "Filesystem format = failed");
        }
        jsonLogMessage(logBuff, WEBINTERFACE);
        strcpy(logBuff, "");
        parseOK = true;
      }
      if (strcmp(p->value().c_str(), "reboot") == 0 ) {
        
        shouldReboot = true;
        jsonLogMessage(F("Reboot requested"), WEBINTERFACE);

        parseOK = true;
      }
#if defined (__HW_VERSION_TWO__)     
      if (strcmp(p->value().c_str(), "level0") == 0 ) {
        
        debugLevel = 0;
        
        sprintf(logBuff, "Debug level = %d", debugLevel);
        jsonLogMessage(logBuff, WEBINTERFACE);
        strcpy(logBuff, "");
        parseOK = true;
      }
      if (strcmp(p->value().c_str(), "level1") == 0 ) {
        
        debugLevel = 1;
        
        sprintf(logBuff, "Debug level = %d", debugLevel);
        jsonLogMessage(logBuff, WEBINTERFACE);
        strcpy(logBuff, "");
        parseOK = true;
      }        
      if (strcmp(p->value().c_str(), "level2") == 0 ) {
        
        debugLevel = 2;
        
        sprintf(logBuff, "Debug level = %d", debugLevel);
        jsonLogMessage(logBuff, WEBINTERFACE);
        strcpy(logBuff, "");
        parseOK = true;
      }
      if (strcmp(p->value().c_str(), "level3") == 0 ) {
        
        debugLevel = 3;
        
        sprintf(logBuff, "Debug level = %d", debugLevel);
        jsonLogMessage(logBuff, WEBINTERFACE);
        strcpy(logBuff, "");
        parseOK = true;
      }      
#endif  
    }      
    else if(strcmp(p->name().c_str(), "command") == 0) {
      if (strcmp(p->value().c_str(), "low") == 0 ) {
        parseOK = true;
        nextIthoVal = systemConfig.itho_low;
        updateItho = true;
      }
      else if (strcmp(p->value().c_str(), "medium") == 0 ) {
        parseOK = true;
        nextIthoVal = systemConfig.itho_medium;
        updateItho = true;
      }
      else if (strcmp(p->value().c_str(), "high") == 0 ) {
        parseOK = true;
        nextIthoVal = systemConfig.itho_high;
        updateItho = true;      
      }
      else if (strcmp(p->value().c_str(), "timer1") == 0 ) {
        parseOK = true;
        nextIthoVal = systemConfig.itho_high;
        nextIthoTimer = systemConfig.itho_timer1;
        updateItho = true;  
      }
      else if (strcmp(p->value().c_str(), "timer2") == 0 ) {
        parseOK = true;
        nextIthoVal = systemConfig.itho_high;
        nextIthoTimer = systemConfig.itho_timer2;
        updateItho = true;  
      }
      else if (strcmp(p->value().c_str(), "timer3") == 0 ) {
        parseOK = true;
        nextIthoVal = systemConfig.itho_high;
        nextIthoTimer = systemConfig.itho_timer3;
        updateItho = true;  
      }
      else if (strcmp(p->value().c_str(), "clearqueue") == 0 ) {
        parseOK = true;
        clearQueue = true;
      }
    }
    else if(strcmp(p->name().c_str(), "speed") == 0) {
      uint16_t val = strtoul (p->value().c_str(), NULL, 10);
      if (val > 0 && val < 255) {
        parseOK = true;
        nextIthoVal = val;
        updateItho = true;        
      }    
      else if (strcmp(p->value().c_str(), "0") == 0 ) {
        parseOK = true;
        nextIthoVal = 0;
        updateItho = true;
      }
  
    }
    else if(strcmp(p->name().c_str(), "timer") == 0) {
      uint16_t timer = strtoul (p->value().c_str(), NULL, 10);
      if (timer > 0 && timer < 65535) {
        parseOK = true;
        nextIthoTimer = timer;
        updateItho = true;        
      }    
    }  

  }


  if(parseOK) {
    request->send(200, "text/html", "OK");
  }
  else {
    request->send(200, "text/html", "NOK");
  }
  
}

void handleDebug(AsyncWebServerRequest *request) {
  if (strcmp(systemConfig.syssec_web, "on") == 0) {
    if (!request->authenticate(systemConfig.sys_username, systemConfig.sys_password))
      return request->requestAuthentication();          
  }  
  AsyncResponseStream *response = request->beginResponseStream("text/html");

  response->print(F("<div class=\"header\"><h1>Debug page</h1></div><br><br>"));
  response->print(F("<div>Config version: "));
  response->print(CONFIG_VERSION);
  response->print(F("<br><br><span>Itho I2C connection status: </span><span id=\'i2cstat\'>unknown</span></div>"));
  response->print(F("<br><span>File system: </span><span>"));
#if defined (__HW_VERSION_ONE__)
  SPIFFS.info(fs_info);
  response->print(fs_info.usedBytes);
#elif defined (__HW_VERSION_TWO__)
  response->print(SPIFFS.usedBytes());
#endif
  response->print(F(" bytes used / "));
#if defined (__HW_VERSION_ONE__)
  response->print(fs_info.totalBytes);
#elif defined (__HW_VERSION_TWO__)
  response->print(SPIFFS.totalBytes());
#endif   
  response->print(F(" bytes total</span><br><a href='#' class='pure-button' onclick=\"$('#main').empty();$('#main').append( html_edit );\">Edit filesystem</a>"));
#if defined (__HW_VERSION_TWO__)
  response->print(F("<br><br><span>CC1101 task memory: </span><span>"));
  response->print(TaskCC1101HWmark);
  response->print(F(" bytes free</span>"));
  response->print(F("<br><span>MQTT task memory: </span><span>"));
  response->print(TaskMQTTHWmark);
  response->print(F(" bytes free</span>"));    
  response->print(F("<br><span>Web task memory: </span><span>"));
  response->print(TaskWebHWmark);
  response->print(F(" bytes free</span>"));
  response->print(F("<br><span>Config and Log task memory: </span><span>"));
  response->print(TaskConfigAndLogHWmark);
  response->print(F(" bytes free</span>"));  
  response->print(F("<br><span>SysControl task memory: </span><span>"));
  response->print(TaskSysControlHWmark);
  response->print(F(" bytes free</span></div>"));
#endif 
  response->print(F("<br><br><div id='syslog_outer'><div style='display:inline-block;vertical-align:top;overflow:hidden;padding-bottom:5px;'>System Log:</div>"));
  
  response->print(F("<div style='padding:10px;background-color:black;min-height:30vh;max-height:60vh;font: 0.9rem Inconsolata, monospace;border-radius:7px;overflow:auto'>"));
  char link[24] = "";
  char linkcur[24] = "";

  if ( SPIFFS.exists("/logfile0.current.log") ) {
    strlcpy(linkcur, "/logfile0.current.log", sizeof(linkcur));
    strlcpy(link, "/logfile1.log", sizeof(link));
  }
  else {
    strlcpy(linkcur, "/logfile1.current.log", sizeof(linkcur));
    strlcpy(link, "/logfile0.log", sizeof(link));      
  }

  File file = SPIFFS.open(linkcur, FILE_READ);
  while (file.available()) {
    if(char(file.peek()) == '\n') response->print("<br>");
    response->print(char(file.read()));
  }
  file.close();

  response->print(F("</div><div style='padding-top:5px;'><a class='pure-button' href='/curlog'>Download current logfile</a>"));

  if ( SPIFFS.exists(link) ) {
    response->print(F("&nbsp;<a class='pure-button' href='/prevlog'>Download previous logfile</a>"));

  }

#if defined (__HW_VERSION_TWO__)  
  response->print(F("</div></div><br><br><div id='rflog_outer' class='hidden'><div style='display:inline-block;vertical-align:top;overflow:hidden;padding-bottom:5px;'>RF Log:</div>"));
  response->print(F("<div id='rflog' style='padding:10px;background-color:black;min-height:30vh;max-height:60vh;font: 0.9rem Inconsolata, monospace;border-radius:7px;overflow:auto'>"));
  response->print(F("</div><div style='padding-top:5px;'><a href='#' class='pure-button' onclick=\"$('#rflog').empty()\">Clear</a></div></div></div><br><br>"));
#endif
  
  request->send(response);
  
  
  DelayedReq.once(1, []() { sysStatReq = true; });

}

void handleCurLogDownload(AsyncWebServerRequest *request) {
  if (strcmp(systemConfig.syssec_web, "on") == 0) {
    if (!request->authenticate(systemConfig.sys_username, systemConfig.sys_password))
      return request->requestAuthentication();          
  }  
  char link[24] = "";
  if (  SPIFFS.exists("/logfile0.current.log") ) {
    strlcpy(link, "/logfile0.current.log", sizeof(link));
  }
  else {
    strlcpy(link, "/logfile1.current.log", sizeof(link));
  }  
  request->send(SPIFFS, link, "", true);
}

void handlePrevLogDownload(AsyncWebServerRequest *request) {
  if (strcmp(systemConfig.syssec_web, "on") == 0) {
    if (!request->authenticate(systemConfig.sys_username, systemConfig.sys_password))
      return request->requestAuthentication();          
  }  
  char link[24] = "";
  if (  SPIFFS.exists("/logfile0.current.log") ) {
    strlcpy(link, "/logfile1.log", sizeof(link));
  }
  else {
     strlcpy(link, "/logfile0.log", sizeof(link)); 
  }  
  request->send(SPIFFS, link, "", true);
}
