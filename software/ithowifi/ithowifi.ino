#define DEBUG_BUILD 1
#define SDAPIN      4
#define SCLPIN      5
#define WIFILED     2
#define STATUSLED   16

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <WebSocketsServer.h>
#include <Wire.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>
#include <PubSubClient.h>

// Initialize classes
MDNSResponder mdns;
ESP8266WiFiMulti WiFiMulti;
ESP8266WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);
WiFiClient espClient;
PubSubClient mqttClient(espClient);

// WiFi Configuration (hard-coded at the moment)
static const char ssid[] = "ssid";
static const char password[] = "password";
// MQTT Settings
const char* mqtt_server = "192.168.0.123";
const int mqtt_port = 1883;
const char* mqtt_user = "";
const char* mqtt_password = "";
const char* mqtt_state_topic = "itho/state";
const char* mqtt_set_topic = "itho/set";

// mDNS hostname for this device
static const char dns_hostname[] = "itho";

// Global variables
volatile uint16_t itho_target   = 0;
char strbuf[80] = "";
const byte mqttDebug = 1;
long lastReconnectAttempt = 0;

// Static function definitions
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length);
void handleRoot();
void handleNotFound();
static void writeIthoVal(uint16_t value);
void receiveEvent(size_t howMany);
void setupMQTTClient();
boolean reconnect();
void mqttCallback(char* topic, byte* payload, unsigned int length);
void updateState(int newState);


// -- BEGIN Index.html
static const char PROGMEM INDEX_HTML[] = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta name = "viewport" content = "width = device-width, initial-scale = 1.0, maximum-scale = 1.0, user-scalable=0">
<title>IoT itho control</title>
<style>
body { background: #262830; font-family: Arial, Helvetica, Sans-Serif; Color: #fffff9; }
#container { width: 80%; max-width: 450px; margin: auto; }
.fan { display: block; clear: none; width: 32px; height: 32px; padding-bottom: 0.5em; background-attachment: fixed; background-position: center; background-repeat: no-repeat; }
.fanOn { float: right; }
.fanOff{ float: left; }
h1 {  display: block; font-size: 2em; margin-top: 0.67em; margin-bottom: 0.67em; margin-left: 0; margin-right: 0; font-weight: bold; text-align: center; }
.slidecontainer {width: 100%; }
.slider {width: 100%; margin: 0 0 3em 0; }
.buttonOff { background-color: #f44336; }
.buttonOn{ background-color: #4CAF50; }
a { background-color: #212121; border: none; color: white; padding: 15px 32px; margin-right: 1em; text-align: center; text-decoration: none; display: inline-block; font-size: 16px; border-radius: 4px; }
</style>
<script>
var websock;
function start() {
websock = new WebSocket('ws://' + window.location.hostname + ':81/');
websock.onopen = function(evt) { console.log('websock open'); };
websock.onclose = function(evt) { console.log('websock close'); };
websock.onerror = function(evt) { console.log(evt); };
websock.onmessage = function(evt) {
console.log(evt);
var itho = document.getElementById('itho');
var ch_values = JSON.parse(evt.data);
itho.value = ch_values.itho;
};
}
function setAll(val) {
var dat = 'itho:'+ val
websock.send(dat);
}
function updateSlider(e) {
var dat = e.id +':'+ e.value
websock.send(dat);
}
</script>
</head>
<body onload="javascript:start();">
<div id="container">
<h1>Itho controller</h1>
<div class="slidecontainer">
<div class="fan fanOn">
<svg style="width:24px;height:24px" viewBox="0 0 24 24">
    <path fill="currentColor" d="M12,11A1,1 0 0,0 11,12A1,1 0 0,0 12,13A1,1 0 0,0 13,12A1,1 0 0,0 12,11M12.5,2C17,2 17.11,5.57 14.75,6.75C13.76,7.24 13.32,8.29 13.13,9.22C13.61,9.42 14.03,9.73 14.35,10.13C18.05,8.13 22.03,8.92 22.03,12.5C22.03,17 18.46,17.1 17.28,14.73C16.78,13.74 15.72,13.3 14.79,13.11C14.59,13.59 14.28,14 13.88,14.34C15.87,18.03 15.08,22 11.5,22C7,22 6.91,18.42 9.27,17.24C10.25,16.75 10.69,15.71 10.89,14.79C10.4,14.59 9.97,14.27 9.65,13.87C5.96,15.85 2,15.07 2,11.5C2,7 5.56,6.89 6.74,9.26C7.24,10.25 8.29,10.68 9.22,10.87C9.41,10.39 9.73,9.97 10.14,9.65C8.15,5.96 8.94,2 12.5,2Z" />
</svg>
</div>
<div class="fan fanOff">
<svg style="width:24px;height:24px" viewBox="0 0 24 24">
    <path fill="currentColor" d="M12.5,2C9.64,2 8.57,4.55 9.29,7.47L15,13.16C15.87,13.37 16.81,13.81 17.28,14.73C18.46,17.1 22.03,17 22.03,12.5C22.03,8.92 18.05,8.13 14.35,10.13C14.03,9.73 13.61,9.42 13.13,9.22C13.32,8.29 13.76,7.24 14.75,6.75C17.11,5.57 17,2 12.5,2M3.28,4L2,5.27L4.47,7.73C3.22,7.74 2,8.87 2,11.5C2,15.07 5.96,15.85 9.65,13.87C9.97,14.27 10.4,14.59 10.89,14.79C10.69,15.71 10.25,16.75 9.27,17.24C6.91,18.42 7,22 11.5,22C13.8,22 14.94,20.36 14.94,18.21L18.73,22L20,20.72L3.28,4Z" />
</svg>
</div>
<input id="itho" type="range" min="0" max="254" value="0" class="slider" onchange="updateSlider(this)">
</div>
<div class="slidecontainer" style="text-align: center">
<a href="#" class="buttonOff" onClick="setAll(0)">Low</a> <a href="#" onClick="setAll(127)">Set to 50%</a> <a href="#" class="buttonOn"  onClick="setAll(254)">High</a>
</div>
</div>
</body>
</html>
)rawliteral";
// -- END Index.html

// WebSocket Event Handler
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  char buffer[160] = {0};
  String content((char * ) payload);
  #if DEBUG_BUILD
  Serial.printf("webSocketEvent(%d, %d, ...)\r\n", num, type);
  #endif
  switch (type) {
  case WStype_DISCONNECTED:
    #if DEBUG_BUILD
    Serial.printf("[%u] Disconnected!\r\n", num);
    #endif
    break;
    // --When new connection is established
  case WStype_CONNECTED:
    {
      IPAddress ip = webSocket.remoteIP(num);
      sprintf(buffer, "{\"itho\":%d,\"i2cstate\":\"%s\"}", itho_target, strbuf);
      #if DEBUG_BUILD
      Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\r\n", num, ip[0], ip[1], ip[2], ip[3], payload);
      #endif
      // Send the current LED status
      webSocket.sendTXT(num, buffer, strlen(buffer));
    }
    break;
    // -- This is what we get from WebSocket
  case WStype_TEXT:
    #if DEBUG_BUILD
    Serial.printf("[%u] get Text: %s\r\n", num, payload);
    #endif

    // Check if data is for itho
    if (content.indexOf("itho:") >= 0) {
      content.remove(0, 5); // Remove first 4 characters
      #if DEBUG_BUILD
      Serial.print("Itho value: ");
      Serial.println(content);
      #endif
      writeIthoVal(content.toInt());
    } else {
      Serial.println("Unknown command?!");
    }

    // send data to all connected clients
    sprintf(buffer, "{\"itho\":%d}", itho_target);
    webSocket.broadcastTXT(buffer, strlen(buffer));
    break;
    // -- Binary?
  case WStype_BIN:
    #if DEBUG_BUILD
    Serial.printf("[%u] get binary length: %u\r\n", num, length);
    #endif
    //hexdump(payload, length);

    // echo data back to browser
    webSocket.sendBIN(num, payload, length);
    break;
  case WStype_ERROR:
    #if DEBUG_BUILD
    Serial.printf("WStype_ERROR [%u]\r\n", num);
    #endif
    break;
  default:
    #if DEBUG_BUILD
    Serial.printf("Invalid WStype [%d]\r\n", type);
    #endif
    break;
  }
}

// Function handler when http root is requested
void handleRoot() {
  server.send_P(200, "text/html", INDEX_HTML);
}

// Function handler for invalid (404) page
void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

// Update itho Value
static void writeIthoVal(uint16_t value) {
  #if DEBUG_BUILD
  Serial.printf("Itho val=%d\n", value);
  #endif

  if (value > 254) {
    #if DEBUG_BUILD
    Serial.printf("Invalid value passed for itho fan: %d\n!", value);
    #endif
    value = 254;
  }

  if (itho_target != value) {
    itho_target = value;

    while (digitalRead(SCLPIN) == LOW) { //'check' if other master is active
      yield();
    }

    Wire.beginTransmission(byte(0x00));
    delay(10);
    //write start of message
    Wire.write(byte(0x60));
    Wire.write(byte(0xC0));
    Wire.write(byte(0x20));
    Wire.write(byte(0x01));
    Wire.write(byte(0x02));

    uint8_t b = (uint8_t) value;
    uint8_t h = 0 - (67 + b);

    Wire.write(b);
    Wire.write(byte(0x00));
    Wire.write(h);

    Wire.endTransmission(true);
    updateState(value);
  }

}
void receiveEvent(size_t howMany) {

  (void) howMany;
  while (3 < Wire.available()) { // loop through all but the last 3
    char c = Wire.read(); // receive byte as a character
    #if DEBUG_BUILD
    Serial.print(c); // print the character
    #endif
  }
  char received[3];
  received[0] = Wire.read();
  received[1] = Wire.read();
  received[2] = Wire.read();

  if (received[0] == 0x00 && received[1] == 0x00 && received[2] == 0xBE) {
    strcat(strbuf, "iOK");
    while (digitalRead(SCLPIN) == LOW) {
      yield();
    }
    Wire.beginTransmission(byte(0x41));
    //write response to itho fan
    Wire.write(byte(0xEF));
    Wire.write(byte(0xC0));
    Wire.write(byte(0x00));
    Wire.write(byte(0x01));
    Wire.write(byte(0x06));
    Wire.write(byte(0x00));
    Wire.write(byte(0x00));
    Wire.write(byte(0x09));
    Wire.write(byte(0x00));
    Wire.write(byte(0x09));
    Wire.write(byte(0x00));
    Wire.write(byte(0xB6));
    Wire.endTransmission();
  } else if (received[0] == 0x60 && received[1] == 0x00 && received[2] == 0x4E) {
    //response OK, init phase done. start rest of code.
    strcat(strbuf, "sOK");
    
    digitalWrite(STATUSLED, HIGH);
    //disable slave mode by detaching interrupts
    detachInterrupt(SDAPIN);
    detachInterrupt(SCLPIN);

  } else {
    //unknown message
    strcat(strbuf, "UNK");
    strcat(strbuf, received);
  }

}

void setupMQTTClient() {
  int connectResult;
  
  if (mqtt_server != "") {
    #if DEBUG_BUILD
    Serial.print("MQTT client: ");
    #endif
    mqttClient.setServer(mqtt_server, mqtt_port);
    mqttClient.setCallback(mqttCallback);
    
    if (mqtt_user == "") {
      connectResult = mqttClient.connect("ESPITHO");
    }
    else {
      connectResult = mqttClient.connect("ESPITHO", mqtt_user, mqtt_password);
    }
    
    if (connectResult) {
      #if DEBUG_BUILD
      Serial.println("Connected");
      #endif
    }
    else {
      #if DEBUG_BUILD
      Serial.print("Failed (");
      Serial.print(mqttClient.state());
      Serial.println(")");
      #endif
    }
    
    if (mqttClient.connected()) {
      #if DEBUG_BUILD
      Serial.print("MQTT topic '");
      Serial.print(mqtt_state_topic);
      #endif
      if (mqttClient.subscribe(mqtt_state_topic)) {
        #if DEBUG_BUILD
        Serial.println("': Subscribed");
        #endif
      }
      else {
        #if DEBUG_BUILD
        Serial.print("': Failed");
        #endif
      }
      #if DEBUG_BUILD
      Serial.print("MQTT topic '");
      Serial.print(mqtt_set_topic);
      #endif
      if (mqttClient.subscribe(mqtt_set_topic)) {
        #if DEBUG_BUILD
        Serial.println("': Subscribed");
        #endif
      }
      else {
        #if DEBUG_BUILD
        Serial.print("': Failed");
        #endif
      }
      
    }
  }
}

//MQTT Reconnection code
boolean reconnect() {
  #if DEBUG_BUILD
  Serial.print("MQTT connection lost. Reconnecting...");
  #endif
  setupMQTTClient();
  return mqttClient.connected();
} 

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  char c_payload[length];
  memcpy(c_payload, payload, length);
  c_payload[length] = '\0';
  
  String s_topic = String(topic);
  String s_payload = String(c_payload);
  
  if (mqttDebug) {
    #if DEBUG_BUILD
    Serial.print("MQTT in: ");
    Serial.print(s_topic);
    Serial.print(" = ");
    Serial.print(s_payload);
    #endif
  }

  if (s_topic == mqtt_set_topic) {
    if (mqttDebug) { 
      #if DEBUG_BUILD
      Serial.println("");
      #endif
    }
    
    if (s_payload.toInt() != itho_target) { writeIthoVal((uint16_t)s_payload.toInt()); }
  }
  else {
    if (mqttDebug) { 
      #if DEBUG_BUILD
      Serial.println(" [unknown message]");
      #endif
    }
  }
}

void updateState(int newState) {

  if (mqttClient.connected()) {
    
    if (mqttDebug) {
      #if DEBUG_BUILD
      Serial.print("MQTT out: ");
      Serial.print(mqtt_state_topic);
      Serial.print(" = ");
      Serial.println(newState);
      #endif
    }
    String payload = String(newState);
    mqttClient.publish(mqtt_state_topic, payload.c_str(), true);
  }
}

//Setup ESP8266
void setup() {
  pinMode(WIFILED, OUTPUT);
  digitalWrite(WIFILED, HIGH);
  pinMode(STATUSLED, OUTPUT);
  digitalWrite(STATUSLED, LOW);

  #if DEBUG_BUILD
  Serial.begin(115200);
  Serial.print("Booting...\r\n");
  Serial.println("Setting up I2C");
  #endif
  
  Wire.begin(SDAPIN, SCLPIN, 0);
  Wire.onReceive(receiveEvent);

  WiFiMulti.addAP(ssid, password);

  while (WiFiMulti.run() != WL_CONNECTED) {
    #if DEBUG_BUILD
    Serial.print(".");
    #endif
    delay(100);
  }
  digitalWrite(WIFILED, LOW);
  
  #if DEBUG_BUILD
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  #endif

  // Setup mDNS to allow accessing this unit through http://DNS_NAME.local
  if (MDNS.begin(dns_hostname)) {
    #if DEBUG_BUILD
    Serial.println("MDNS responder started");
    #endif
    MDNS.addService("http", "tcp", 80);
    MDNS.addService("ws", "tcp", 81);
  } else {
    #if DEBUG_BUILD
    Serial.println("MDNS.begin failed");
    #endif
  }
  #if DEBUG_BUILD
  Serial.printf("Connect to http://%s.local or http://", dns_hostname);
  Serial.println(WiFi.localIP());
  #endif
  
  server.on("/", handleRoot);
  server.onNotFound(handleNotFound);

  server.begin();

  webSocket.begin();
  webSocket.onEvent(webSocketEvent);

  setupMQTTClient();
  
  #if DEBUG_BUILD
  Serial.println("Setup done!");
  #endif

}

// Main()
void loop() {
  MDNS.update();
  webSocket.loop();
  ArduinoOTA.handle();
  server.handleClient();

  // handle MQTT:
  if (mqttClient.connected()) {
    mqttClient.loop();
  }
  else {
    long now = millis();
    if (now - lastReconnectAttempt > 5000) {
      lastReconnectAttempt = now;
      // Attempt to reconnect
      if (reconnect()) {
        lastReconnectAttempt = 0;
      }
    }
  }  

}
