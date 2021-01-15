
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
  for(int i=0;i<params;i++){
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
        }
        else {
          strcpy(logBuff, "Filesystem format = failed");
        }
        jsonMessageBox(logBuff, "");
        strcpy(logBuff, "");
        parseOK = true;
      }
      if (strcmp(p->value().c_str(), "reboot") == 0 ) {
        
        shouldReboot = true;
        jsonMessageBox("Reboot requested", "");

        parseOK = true;
      }
#if defined (__HW_VERSION_TWO__)     
      if (strcmp(p->value().c_str(), "level0") == 0 ) {
        
        debugLevel = 0;
        
        sprintf(logBuff, "Debug level = %d", debugLevel);
        jsonMessageBox(logBuff, "");
        strcpy(logBuff, "");
        parseOK = true;
      }
      if (strcmp(p->value().c_str(), "level1") == 0 ) {
        
        debugLevel = 1;
        
        sprintf(logBuff, "Debug level = %d", debugLevel);
        jsonMessageBox(logBuff, "");
        strcpy(logBuff, "");
        parseOK = true;
      }        
      if (strcmp(p->value().c_str(), "level2") == 0 ) {
        
        debugLevel = 2;
        
        sprintf(logBuff, "Debug level = %d", debugLevel);
        jsonMessageBox(logBuff, "");
        strcpy(logBuff, "");
        parseOK = true;
      }
      if (strcmp(p->value().c_str(), "level3") == 0 ) {
        
        debugLevel = 3;
        
        sprintf(logBuff, "Debug level = %d", debugLevel);
        jsonMessageBox(logBuff, "");
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
  AsyncResponseStream *response = request->beginResponseStream("text/html");

  response->print("<div class=\"header\"><h1>Debug page</h1></div><br><br>");
  response->print("<div>Config version: ");
  response->print(CONFIG_VERSION);
  response->print("<br><br><span>Itho I2C connection status: </span><span id=\'i2cstat\'>unknown</span><br><br></div>");
  response->print("<div style='padding: 10px;background-color: black;background-image: radial-gradient(rgba(0, 150, 0, 0.55), black 140%);height: 60vh;}  color: white;  font: 0.9rem Inconsolata, monospace;border-radius: 10px;overflow:auto'>--- System Log ---<br>");
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
    //count row
    //of more than x row display fom total - 1
    response->print(char(file.read()));
   
  }
  file.close();

  response->print("</div><br><br><a class='pure-button' href='/curlog'>Download current logfile</a>");

  if ( SPIFFS.exists(link) ) {
    response->print("&nbsp;<a class='pure-button' href='/prevlog'>Download previous logfile</a>");

  }
  response->print("<br><br>");
  
  
  request->send(response);
  
  
  DelayedReq.once(1, []() { sysStatReq = true; });

}

void handleCurLogDownload(AsyncWebServerRequest *request) {
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
  char link[24] = "";
  if (  SPIFFS.exists("/logfile0.current.log") ) {
    strlcpy(link, "/logfile1.log", sizeof(link));
  }
  else {
     strlcpy(link, "/logfile0.log", sizeof(link)); 
  }  
  request->send(SPIFFS, link, "", true);
}
