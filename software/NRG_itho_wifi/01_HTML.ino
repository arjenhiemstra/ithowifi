

#include <ESPAsyncWebServer.h>

const char* html_mainpage = R"=====(
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
    <style>.ithoset{padding: .3em .2em !important;}.dot-elastic{position:relative;width:6px;height:6px;border-radius:5px;background-color:#039;color:#039;animation:dotElastic 1s infinite linear}.dot-elastic::after,.dot-elastic::before{content:'';display:inline-block;position:absolute;top:0}.dot-elastic::before{left:-15px;width:6px;height:6px;border-radius:5px;background-color:#039;color:#039;animation:dotElasticBefore 1s infinite linear}.dot-elastic::after{left:15px;width:6px;height:6px;border-radius:5px;background-color:#039;color:#039;animation:dotElasticAfter 1s infinite linear}@keyframes dotElasticBefore{0%{transform:scale(1,1)}25%{transform:scale(1,1.5)}50%{transform:scale(1,.67)}75%{transform:scale(1,1)}100%{transform:scale(1,1)}}@keyframes dotElastic{0%{transform:scale(1,1)}25%{transform:scale(1,1)}50%{transform:scale(1,1.5)}75%{transform:scale(1,1)}100%{transform:scale(1,1)}}@keyframes dotElasticAfter{0%{transform:scale(1,1)}25%{transform:scale(1,1)}50%{transform:scale(1,.67)}75%{transform:scale(1,1.5)}100%{transform:scale(1,1)}}</style>
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
                <li class="pure-menu-item"><a href="status" class="pure-menu-link">Itho status</a></li>                
                <li id="remotemenu" class="pure-menu-item hidden"><a href="remotes" class="pure-menu-link">RF remotes</a></li>
                <li class="pure-menu-item"><a href="vremotes" class="pure-menu-link">Virtual remotes</a></li>
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

  int params = request->params();
  
  if (systemConfig.syssec_api) {
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


  const char * speed = nullptr;
  const char * timer = nullptr;
  const char * vremotecmd = nullptr;
  const char * vremoteidx = nullptr;
  const char * vremotename = nullptr;
  
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
        StaticJsonDocument<1000> root;
        JsonObject obj = root.to<JsonObject>(); // Fill the object
        ithoQueue.get(obj);
        obj["ithoSpeed"] = ithoQueue.ithoSpeed;
        obj["ithoOldSpeed"] = ithoQueue.ithoOldSpeed;
        obj["fallBackSpeed"] = ithoQueue.fallBackSpeed;
        serializeJson(root, *response);
        request->send(response);
        
        return;
      }
      else if (strcmp(p->value().c_str(), "ithostatus") == 0 ) {
        AsyncResponseStream *response = request->beginResponseStream("text/html");        
        DynamicJsonDocument doc(6000);
        
        if (doc.capacity() == 0) {
          logInput("MQTT: JsonDocument memory allocation failed (html api)");
          return;
        }
        
        JsonObject root = doc.to<JsonObject>();
        getIthoStatusJSON(root);
        
        serializeJson(root, *response);
        request->send(response);
        
        return;
      }
      else if (strcmp(p->value().c_str(), "lastcmd") == 0 ) {
        AsyncResponseStream *response = request->beginResponseStream("text/html");        
        DynamicJsonDocument doc(1000);
        
        JsonObject root = doc.to<JsonObject>();
        getLastCMDinfoJSON(root);
        
        serializeJson(root, *response);
        request->send(response);
        
        return;
      }      
      else if (strcmp(p->value().c_str(), "remotesinfo") == 0 ) {
        AsyncResponseStream *response = request->beginResponseStream("text/html");        
        DynamicJsonDocument doc(2000);
                
        JsonObject root = doc.to<JsonObject>();
        
        getRemotesInfoJSON(root);
        
        serializeJson(root, *response);
        request->send(response);
        
        return;
      }         
    }
    else if(strcmp(p->name().c_str(), "command") == 0) {
      parseOK = ithoExecCommand(p->value().c_str(), HTMLAPI);
    }
    else if(strcmp(p->name().c_str(), "vremote") == 0) {
      vremotecmd = p->value().c_str();
    }
    else if(strcmp(p->name().c_str(), "vremotecmd") == 0) {
      vremotecmd = p->value().c_str();
    }    
    else if(strcmp(p->name().c_str(), "vremoteindex") == 0) {
      vremoteidx = p->value().c_str();
    }
    else if(strcmp(p->name().c_str(), "vremotename") == 0) {
      vremotename = p->value().c_str();
    }        
    else if(strcmp(p->name().c_str(), "speed") == 0) {
      speed = p->value().c_str();
    }
    else if(strcmp(p->name().c_str(), "timer") == 0) {
      timer = p->value().c_str();
    }
    else if(strcmp(p->name().c_str(), "debug") == 0) {
      if (strcmp(p->value().c_str(), "level0") == 0 ) {
        setRFdebugLevel(0);
        parseOK = true;
      }
      if (strcmp(p->value().c_str(), "level1") == 0 ) {
        setRFdebugLevel(1);
        parseOK = true;
      }        
      if (strcmp(p->value().c_str(), "level2") == 0 ) {
        setRFdebugLevel(2);
        parseOK = true;
      }
      if (strcmp(p->value().c_str(), "level3") == 0 ) {
        setRFdebugLevel(3);
        parseOK = true;
      }
      if (strcmp(p->value().c_str(), "reboot") == 0 ) {
        shouldReboot = true;
        logMessagejson("Reboot requested", WEBINTERFACE);
        parseOK = true;
      }
    }
  }
  if( vremotecmd != nullptr && (vremoteidx == nullptr && vremotename == nullptr) ) {
    parseOK = ithoI2CCommand(0, vremotecmd, HTMLAPI);
  }
  else {
    //exec command for specific vremote
    int index = -1;
    if(vremotename != nullptr) {
      index = virtualRemotes.getRemoteIndexbyName(vremotename);
    }
    else {
      if(strcmp(vremoteidx, "0") == 0) {
        index = 0;
      }
      else {
        index = atoi(vremoteidx);
        if (index == 0) index = -1;
      }
    }
    if (index == -1) parseOK = false;
    else {
      parseOK = ithoI2CCommand(index, vremotecmd, HTMLAPI);
    }
  }
  //check speed & timer
  if(speed != nullptr && timer != nullptr) {
    parseOK = ithoSetSpeedTimer(speed, timer, HTMLAPI);
  }
  else if(speed != nullptr && timer == nullptr) {
    parseOK = ithoSetSpeed(speed, HTMLAPI);
  }
  else if(timer != nullptr && speed == nullptr) {  
    parseOK = ithoSetTimer(timer, HTMLAPI);
  }
  if(parseOK) {
    request->send(200, "text/html", "OK");
  }
  else {
    request->send(200, "text/html", "NOK");
  }
  
}

void handleDebug(AsyncWebServerRequest *request) {
  if (systemConfig.syssec_web) {
    if (!request->authenticate(systemConfig.sys_username, systemConfig.sys_password))
      return request->requestAuthentication();          
  }  
  AsyncResponseStream *response = request->beginResponseStream("text/html");

  response->print("<div class=\"header\"><h1>Debug page</h1></div><br><br>");
  response->print("<div>Config version: ");
  response->print(CONFIG_VERSION);
  response->print("<br><br><span>Itho I2C connection status: </span><span id=\'ithoinit\'>unknown</span></div>");
  
  response->print("<br><span>File system: </span><span>");

  response->print(ACTIVE_FS.usedBytes());
  response->print(" bytes used / ");
  response->print(ACTIVE_FS.totalBytes());
  response->print(" bytes total</span><br><a href='#' class='pure-button' onclick=\"$('#main').empty();$('#main').append( html_edit );\">Edit filesystem</a>");
  response->print("<br><br><span>CC1101 task memory: </span><span>");
  response->print(TaskCC1101HWmark);
  response->print(" bytes free</span>");
  response->print("<br><span>MQTT task memory: </span><span>");
  response->print(TaskMQTTHWmark);
  response->print(" bytes free</span>");    
  response->print("<br><span>Web task memory: </span><span>");
  response->print(TaskWebHWmark);
  response->print(" bytes free</span>");
  response->print("<br><span>Config and Log task memory: </span><span>");
  response->print(TaskConfigAndLogHWmark);
  response->print(" bytes free</span>");  
  response->print("<br><span>SysControl task memory: </span><span>");
  response->print(TaskSysControlHWmark);
  response->print(" bytes free</span></div>");
  response->print("<br><br><div id='syslog_outer'><div style='display:inline-block;vertical-align:top;overflow:hidden;padding-bottom:5px;'>System Log:</div>");
  
  response->print("<div style='padding:10px;background-color:black;min-height:30vh;max-height:60vh;font: 0.9rem Inconsolata, monospace;border-radius:7px;overflow:auto'>");
  char link[24] = "";
  char linkcur[24] = "";

  if ( ACTIVE_FS.exists("/logfile0.current.log") ) {
    strlcpy(linkcur, "/logfile0.current.log", sizeof(linkcur));
    strlcpy(link, "/logfile1.log", sizeof(link));
  }
  else {
    strlcpy(linkcur, "/logfile1.current.log", sizeof(linkcur));
    strlcpy(link, "/logfile0.log", sizeof(link));      
  }

  File file = ACTIVE_FS.open(linkcur, FILE_READ);
  while (file.available()) {
    if(char(file.peek()) == '\n') response->print("<br>");
    response->print(char(file.read()));
  }
  file.close();

  response->print("</div><div style='padding-top:5px;'><a class='pure-button' href='/curlog'>Download current logfile</a>");

  if ( ACTIVE_FS.exists(link) ) {
    response->print("&nbsp;<a class='pure-button' href='/prevlog'>Download previous logfile</a>");

  }


  response->print("</div></div><br><br><div id='rflog_outer' class='hidden'><div style='display:inline-block;vertical-align:top;overflow:hidden;padding-bottom:5px;'>RF Log:</div>");
  response->print("<div id='rflog' style='padding:10px;background-color:black;min-height:30vh;max-height:60vh;font: 0.9rem Inconsolata, monospace;border-radius:7px;overflow:auto'>");
  response->print("</div><div style='padding-top:5px;'><a href='#' class='pure-button' onclick=\"$('#rflog').empty()\">Clear</a></div></div></div>");
  response->print("<form class=\"pure-form pure-form-aligned\"><fieldset><legend><br>RF debug mode (only functional with active CC1101 RF module):</legend><br><button id=\"rfdebug-0\" class=\"pure-button pure-button-primary\">Off</button>&nbsp;<button id=\"rfdebug-1\" class=\"pure-button pure-button-primary\">Level1</button>&nbsp;<button id=\"rfdebug-2\" class=\"pure-button pure-button-primary\">Level2</button>&nbsp;<button id=\"rfdebug-3\" class=\"pure-button pure-button-primary\">Level3</button>&nbsp;<br><br>");
  response->print("<form class=\"pure-form pure-form-aligned\"><fieldset><legend><br>Low level itho I2C commands:</legend><br><span>I2C virtual remote commands:</span><br><button id=\"ithobutton-low\" class=\"pure-button pure-button-primary\">Low</button>&nbsp;<button id=\"ithobutton-medium\" class=\"pure-button pure-button-primary\">Medium</button>&nbsp;<button id=\"ithobutton-high\" class=\"pure-button pure-button-primary\">High</button><br><br>");
  response->print("<button id=\"ithobutton-join\" class=\"pure-button pure-button-primary\">Join</button>&nbsp;<button id=\"ithobutton-leave\" class=\"pure-button pure-button-primary\">Leave</button><br><br>");
  response->print("<button id=\"ithobutton-type\" class=\"pure-button pure-button-primary\">Query Devicetype</button><br><span>Result:&nbsp;</span><span id=\'ithotype\'></span><br><br>");
  response->print("<button id=\"ithobutton-statusformat\" class=\"pure-button pure-button-primary\">Query Status Format</button><br><span>Result:&nbsp;</span><span id=\'ithostatusformat\'></span><br><br>");
  response->print("<button id=\"ithobutton-status\" class=\"pure-button pure-button-primary\">Query Status</button><br><span>Result:&nbsp;</span><span id=\'ithostatus\'></span><br><br>");
  response->print("<button id=\"button2410\" class=\"pure-button pure-button-primary\">Query 2410</button>setting index: <input id=\"itho_setting_id\" type=\"number\" min=\"0\" max=\"254\" size=\"6\" value=\"0\"><br><span>Result:&nbsp;</span><span id=\'itho2410\'></span><br><span>Current:&nbsp;</span><span id=\'itho2410cur\'></span><br><span>Minimum value:&nbsp;</span><span id=\'itho2410min\'></span><br><span>Maximum value:&nbsp;</span><span id=\'itho2410max\'></span><br><br>");
  response->print("<span style=\"color:red\">Warning!!<br> \"Set 2410\" changes the settings of your itho unit<br>Use with care and use only if you know what you are doing!</span><br>");
  response->print("<button id=\"button2410set\" class=\"pure-button pure-button-primary\">Set 2410</button>setting index: <input id=\"itho_setting_id_set\" type=\"number\" min=\"0\" max=\"254\" size=\"6\" value=\"0\"> setting value: <input id=\"itho_setting_value_set\" type=\"number\" min=\"-2147483647\" max=\"2147483647\" size=\"10\" value=\"0\"><br><span>Sent command:&nbsp;</span><span id=\'itho2410set\'></span><br><span>Result:&nbsp;</span><span id=\'itho2410setres\'></span><br>");
  response->print("<span style=\"color:red\">Warning!!</span><br><br>");
  response->print("<button id=\"ithobutton-31DA\" class=\"pure-button pure-button-primary\">Query 31DA</button><br><span>Result:&nbsp;</span><span id=\'itho31DA\'></span><br><br>");
  response->print("<button id=\"ithobutton-31D9\" class=\"pure-button pure-button-primary\">Query 31D9</button><br><span>Result:&nbsp;</span><span id=\'itho31D9\'></span><br><br>");
  response->print("<button id=\"ithobutton-10D0\" class=\"pure-button pure-button-primary\">Filter reset</button></fieldset></form><br>");  

  response->print("<br><br>");
  
  request->send(response);
  
  
  DelayedReq.once(1, []() { sysStatReq = true; });

}

void handleCurLogDownload(AsyncWebServerRequest *request) {
  if (systemConfig.syssec_web) {
    if (!request->authenticate(systemConfig.sys_username, systemConfig.sys_password))
      return request->requestAuthentication();          
  }  
  char link[24] = "";
  if (  ACTIVE_FS.exists("/logfile0.current.log") ) {
    strlcpy(link, "/logfile0.current.log", sizeof(link));
  }
  else {
    strlcpy(link, "/logfile1.current.log", sizeof(link));
  }  
  request->send(ACTIVE_FS, link, "", true);
}

void handlePrevLogDownload(AsyncWebServerRequest *request) {
  if (systemConfig.syssec_web) {
    if (!request->authenticate(systemConfig.sys_username, systemConfig.sys_password))
      return request->requestAuthentication();          
  }  
  char link[24] = "";
  if (  ACTIVE_FS.exists("/logfile0.current.log") ) {
    strlcpy(link, "/logfile1.log", sizeof(link));
  }
  else {
     strlcpy(link, "/logfile0.log", sizeof(link)); 
  }  
  request->send(ACTIVE_FS, link, "", true);
}
