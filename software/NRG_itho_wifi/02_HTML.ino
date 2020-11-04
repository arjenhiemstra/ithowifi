
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

</body>
<script src="/js/general.js"></script>
</html>
)=====";


void handleMainpage(AsyncWebServerRequest *request) {
  request->send_P(200, "text/html", html_mainpage);
}

void handleDebug(AsyncWebServerRequest *request) {
  AsyncResponseStream *response = request->beginResponseStream("text/html");

  //saveFileRemotes("/remotes.json", remotes);
  loadFileRemotes("/remotes.json", remotes);
  
  response->print("<div class=\"header\"><h1>Debug page</h1></div><br><br>");
  response->print("<div>Firmware version: ");
  response->print(FWVERSION);
  response->print("<br><br>Config version: ");
  response->print(CONFIG_VERSION);
  response->print("<br><br></div>");
  response->print("<div style='padding: 10px;background-color: black;background-image: radial-gradient(rgba(0, 150, 0, 0.55), black 140%);height: 60vh;}  color: white;  font: 0.9rem Inconsolata, monospace;border-radius: 10px;overflow:auto'>--- System Log ---<br>");

  char bufLinkCur[24];
  char bufLink[24];
  
  if ( SPIFFS.exists("/logfile0.current.log") ) {
    strlcpy(bufLinkCur, "/logfile0.current.log", sizeof(bufLinkCur));
    strlcpy(bufLink, "/logfile1.log", sizeof(bufLink));
  }
  else {
    strlcpy(bufLinkCur, "/logfile1.current.log", sizeof(bufLinkCur));
    strlcpy(bufLink, "/logfile0.log", sizeof(bufLink));
  }

  File file = SPIFFS.open(bufLinkCur, FILE_READ);
  while (file.available()) {
    //count row
    //of more than x row display fom total - 1
    response->print(char(file.read()));
   
  }
  file.close();

  response->print("</div><br><br><a class='pure-button' href='/curlog'>Download current logfile</a>");


  if ( SPIFFS.exists(bufLink) ) {
    response->print("&nbsp;<a class='pure-button' href='/prevlog'>Download previous logfile</a>");

  }
  
  response->print("<br><br>");
  request->send(response);
  
  
}

void handleCurLogDownload(AsyncWebServerRequest *request) {
  String link = "";
  if (  SPIFFS.exists("/logfile0.current.log") ) {
    link = "/logfile0.current.log";
  }
  else {
    link = "/logfile1.current.log";
  }  
  request->send(SPIFFS, link, String(), true);
}

void handlePrevLogDownload(AsyncWebServerRequest *request) {
  String link = "";
  if (  SPIFFS.exists("/logfile0.current.log") ) {
    link = "/logfile1.log";
  }
  else {
     link = "/logfile0.log";  
  }  
  request->send(SPIFFS, link, String(), true);
}
