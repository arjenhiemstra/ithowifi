#define FWVERSION "2.1.2"

#define LOGGING_INTERVAL 21600000
#define ENABLE_FAILSAVE_BOOT

#include "hardware.h"
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

#if defined (ESP8266)
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ESPAsyncTCP.h>
#include <Hash.h>

#elif defined (ESP32)
#include <WiFi.h>
#include <WiFiClient.h>
#include "esp_wifi.h"
#include <ESPmDNS.h>
#include <AsyncTCP.h>
#include "SPIFFS.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_task_wdt.h"
#include "IthoCC1101.h"   // Largly based on and thanks to https://github.com/supersjimmie/IthoEcoFanRFT
#include "IthoPacket.h"   // Largly based on and thanks to https://github.com/supersjimmie/IthoEcoFanRFT
#include "IthoRemote.h"
char debugLog[200];
bool debugLogInput = false;
uint8_t debugLevel = 0;
Ticker LogMessage;
#else
#error "Unsupported hardware"
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
Ticker DelayedSave;
#if defined (__HW_VERSION_ONE__)
FSInfo fs_info;
#elif defined (__HW_VERSION_TWO__)
Ticker timerLearnLeaveMode;
IthoCC1101 rf;
IthoPacket packet;
IthoRemote remotes;
TaskHandle_t CC1101TaskHandle;
#endif
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
unsigned long APmodeTimeout = 0;
unsigned long lastSysMessage = 0;
unsigned long previousUpdate = 0;
unsigned long wifiLedUpdate = 0;
unsigned long lastLog = 0;

//flags used
bool shouldReboot = false;
bool dontSaveConfig = false;
bool clearQueue = false;
bool wifiModeAP = false;
bool sysStatReq = false;
bool runscan = false;
volatile bool updateItho = false;
volatile bool ithoCheck = false;
volatile bool saveRemotes = false;
bool rfInitOK = false;

size_t content_len;

typedef enum { WEBINTERFACE, RFLOG } logtype;

#if defined (__HW_VERSION_TWO__)
IthoCommand RFTcommand[3] = {IthoUnknown, IthoUnknown, IthoUnknown};
byte RFTRSSI[3] = {0, 0, 0};
byte RFTcommandpos = 0;
bool RFTidChk[3] = {false, false, false};
#endif

void dummyFunct() {
  //some weird stuff with the arduino IDE
}
