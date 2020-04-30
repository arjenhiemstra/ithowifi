#define SDAPIN      4
#define SCLPIN      5
#define WIFILED     2
#define STATUSPIN   14

#define FWVERSION "1.2"
#define CONFIG_VERSION "001" //Change when SystemConfig struc changes

#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFSEditor.h>
#include <DNSServer.h>
#include "System.h"
#include <PubSubClient.h>
#include <ArduinoOTA.h>
#include <Wire.h>


#if defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ESPAsyncTCP.h>
#include <Hash.h>
#else
#include <WiFi.h>
#include <WiFiClient.h>
#include <ESPmDNS.h>
#include <AsyncTCP.h>
#include <SPIFFS.h>
#include <FS.h>
#endif

System sys;
WiFiClient client;
DNSServer dnsServer;
PubSubClient mqttClient(client);

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
AsyncEventSource events("/events");

const char* espName = "NRG-itho-";
const char* http_username = "admin";
const char* http_password = "admin";

int FSTotal;
int FSUsed;

int MQTT_conn_state = -5;
int MQTT_conn_state_new = 0;
unsigned long lastMQTTReconnectAttempt = 0;
unsigned long lastWIFIReconnectAttempt = 0;

int newMem;
int memHigh;
int memLow = 1000000;

// Global variables
volatile uint16_t itho_current_val   = 0;
volatile uint16_t itho_new_val   = 0;

char i2cstat[20] = "";

unsigned long loopstart = 0;
unsigned long updatetimer = 0;
unsigned long lastSysMessage = 0;
unsigned long previousUpdate = 0; 
unsigned long wifiLedUpdate = 0; 

//flags used
bool shouldReboot = false;
bool updateItho = false;
bool wifiModeAP = false;
bool sysStatReq = false;
bool runscan = false;
bool mqttSetup = false;

size_t content_len;

const char WiFiAPPSK[] = "password"; //default AP mode password
const char* serverName;
const char* accessToken;

struct WifiConfig {
  char ssid[33];
  char passwd[65];
  char dhcp[5];
  uint8_t renew;
  char ip[16];
  char subnet[16];
  char gateway[16];
  char dns1[16];
  char dns2[16];
  uint8_t port;
};
WifiConfig wifiConfig = { //default config
  "",
  "",
  "on",
  60,
  "192.168.4.1",
  "255.255.255.0",
  "127.0.0.1",
  "8.8.8.8",
  "8.8.4.4",
  80
};


struct SystemConfig {
  char mqtt_active[5];
  char mqtt_serverName[65];
  char mqtt_username[32];
  char mqtt_password[32];
  int mqtt_port;
  int mqtt_version;
  char mqtt_state_topic[128];
  char mqtt_state_retain[5];  
  char mqtt_cmd_topic[128];
  char version_of_program[4];
};

SystemConfig systemConfig = { //default config
  "off", //MQTT active "on"/"off"
  "192.168.1.123", //MQTT URL
  "", //MQTT username
  "", //MQTT password
  1883, //MQTT port
  1, //MQTT version  
  "itho/state", //MQTT state topic
  "yes", //MQTT state retain "yes"/"no"
  "itho/cmd", //MQTT command topic
  CONFIG_VERSION
};

void onRequest(AsyncWebServerRequest *request) {
  //Handle Unknown Request
  request->send(404);
}

void onBody(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
  //Handle body
}

void onUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
  //Handle upload
}
