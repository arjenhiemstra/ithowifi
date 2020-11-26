#define SDAPIN      4
#define SCLPIN      5
#define WIFILED     2
#define STATUSPIN   14
#define LOGGING_INTERVAL 21600000

#define FWVERSION "1.4.1"


#include <ArduinoJson.h>
#include <ArduinoSort.h>
#include <AsyncEventSource.h>
#include <AsyncJson.h>
#include <AsyncWebSocket.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFSEditor.h>
#include <StringArray.h>
#include <WebAuthentication.h>
#include <WebHandlerImpl.h>
#include <WebResponseImpl.h>
#include <DNSServer.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <time.h>
#include <Ticker.h>

#include <ArduinoLog.h>       // https://github.com/thijse/Arduino-Log
#include <SpiffsFilePrint.h>  // https://github.com/PRosenb/SPIFFS_FilePrint

#include "IthoQueue.h"
#include "System.h"
#include "SystemConfig.h"
#include "WifiConfig.h"

#include <ArduinoOTA.h>
#include <FS.h>

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
#include "SPIFFS.h"
#endif


WiFiClient client;
DNSServer dnsServer;
PubSubClient mqttClient(client);

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
AsyncEventSource events("/events");

SpiffsFilePrint filePrint("/logfile", 2, 10000);

Ticker IthoCMD;
Ticker DelayedReq;

IthoQueue ithoQueue;
System sys;
SystemConfig systemConfig;
WifiConfig wifiConfig;


const char* espName = "nrg-itho-";
const char* http_username = "admin";
const char* http_password = "admin";
const char* WiFiAPPSK = "password"; //default AP mode password

// Global variables
int MQTT_conn_state = -5;
int MQTT_conn_state_new = 0;
unsigned long lastMQTTReconnectAttempt = 0;
unsigned long lastWIFIReconnectAttempt = 0;

volatile uint16_t ithoCurrentVal   = 0;
volatile uint16_t nextIthoVal = 0;
volatile unsigned long nextIthoTimer = 0;

char i2cstat[20] = "";
char logBuff[256] = "";

unsigned long loopstart = 0;
unsigned long updatetimer = 0;
unsigned long lastSysMessage = 0;
unsigned long previousUpdate = 0;
unsigned long wifiLedUpdate = 0;
unsigned long lastLog = 0;

//flags used
bool shouldReboot = false;
bool updateItho = false;
bool clearQueue = false;
bool wifiModeAP = false;
bool sysStatReq = false;
bool runscan = false;

size_t content_len;

void dummyFunct() {
  //some weird stuff with the arduino IDE
}
