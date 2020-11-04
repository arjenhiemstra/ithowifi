#define SDAPIN      4
#define SCLPIN      5
#define WIFILED     2
#define STATUSPIN   16
#define ITHO_IRQ_PIN 0
#define LOGGING_INTERVAL 21600000


#define FWVERSION "2.0.0"
#define CONFIG_VERSION "002" //Change when SystemConfig struc changes

#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFSEditor.h>
#include <DNSServer.h>
#include "System.h"
#include <PubSubClient.h>
#include <ArduinoOTA.h>
#include <Wire.h>
#include <time.h>

#include <ArduinoLog.h>       // https://github.com/thijse/Arduino-Log
#include <SpiffsFilePrint.h>  // https://github.com/PRosenb/SPIFFS_FilePrint
#include "IthoCC1101.h"
#include "IthoPacket.h"
#include "IthoRemote.h"

#include "SystemConfig.h"
#include "WifiConfig.h"

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

SpiffsFilePrint filePrint("/logfile", 2, 10000);

IthoCC1101 rf;
IthoPacket packet;
IthoRemote remotes;

SystemConfig systemConfig;
WifiConfig wifiConfig;

 int RFTid0[] = {106, 170, 106, 101, 154, 107, 154, 86};
 int RFTid1[] = {102, 169, 86, 153, 149, 169, 154, 86}; // my ID
 int RFTid2[] = {107, 171, 105, 102, 155, 108, 154, 86};

// Div
bool ITHOhasPacket = false;
bool ITHOisr = false;
long checkin10ms = 0;
bool knownRFid = false;
IthoCommand RFTcommand[3] = {IthoUnknown, IthoUnknown, IthoUnknown};
byte RFTRSSI[3] = {0, 0, 0};
byte RFTcommandpos = 0;
IthoCommand RFTlastCommand = IthoLow;
IthoCommand RFTstate = IthoUnknown;
IthoCommand savedRFTstate = IthoUnknown;
bool RFTidChk[3] = {false, false, false};
String Laststate;
String CurrentState;
char lastidindex[2];


const char* espName = "nrg-itho-";
const char* http_username = "admin";
const char* http_password = "admin";
const char* WiFiAPPSK = "password"; //default AP mode password

int MQTT_conn_state = -5;
int MQTT_conn_state_new = 0;
unsigned long lastMQTTReconnectAttempt = 0;
unsigned long lastWIFIReconnectAttempt = 0;


// Global variables
volatile uint16_t itho_current_val   = 0;
volatile uint16_t itho_new_val      = 0;

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
bool wifiModeAP = false;
bool sysStatReq = false;
bool runscan = false;
bool mqttSetup = false;

size_t content_len;

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
