#define FWVERSION "2.2-beta8-5-gf49be90"

#define LOGGING_INTERVAL 21600000  //Log system status at regular intervals
#define ENABLE_FAILSAVE_BOOT
//#define INFORMATIVE_LOGGING
//#define ENABLE_SERIAL
#define ENABLE_SHT30_SENSOR_SUPPORT

/*
 * 
 * Info to compile the firmware yourself:
 * Build is done using Arduino IDE
 * 
 * Used libs and versions are mentioned below.
 * 
 * uncomment #define ENABLE_SHT30_SENSOR_SUPPORT to include readout support of the built-in hum and temp sensor of some itho boxes. This is experimental and might not work properly!
 * 
 * For HW rev 1:
 * Select 'LOLIN(WEMOS)D1 R2 & mini' as board
 * 
 * For HW rev 2:
 * Select 'ESP32 Dev Module' as board
 * Choose partition scheme: Minimal SPIFFS (1.9 APP with OTA)
 * Because of FreetRTOS function xTaskCreateStaticPinnedToCore a standard arduino esp-32 SDK lib needs to be replaced otherwise the code doesn't compile.
 * A compiled version is included in the folder static files and should be copied to your SDK location ie.:
 * /Arduino15/packages/esp32/hardware/esp32/1.0.5-rc7/tools/sdk/lib
 * and in:
 * /Arduino15/packages/esp32/hardware/esp32/1.0.5-rc7/tools/sdk/include/freertos/freertos/FreeRTOSConfig.h
 * change "#define configSUPPORT_STATIC_ALLOCATION CONFIG_SUPPORT_STATIC_ALLOCATION" to "#define configSUPPORT_STATIC_ALLOCATION 1"
 * 
 * 
 */
 
#include "hardware.h"
#include <ArduinoJson.h>  // https://github.com/bblanchon/ArduinoJson [6.17.3]
#include <ESPAsyncWebServer.h>  // https://github.com/me-no-dev/ESPAsyncWebServer [latest]
#include <SPIFFSEditor.h>       // https://github.com/me-no-dev/ESPAsyncWebServer [latest]
#include <DNSServer.h>
#include <PubSubClient.h>     // https://github.com/arjenhiemstra/PubSubClientStatic [latest], forked from https://github.com/knolleary/pubsubclient and https://github.com/mhmtsui/pubsubclient
#include <Wire.h>
#include <time.h>
#include <Ticker.h>

#include <ArduinoLog.h>       // https://github.com/thijse/Arduino-Log [1.0.3]
#include <SpiffsFilePrint.h>  // https://github.com/PRosenb/SPIFFS_FilePrint [1.0.0]

#include "IthoQueue.h"
#include "System.h"
#include "SystemConfig.h"
#include "WifiConfig.h"

#include <ArduinoOTA.h>
#include <FS.h>
#include "SHTSensor.h"        // https://github.com/Sensirion/arduino-sht [latest]

#if defined (ESP8266)
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ESPAsyncTCP.h>      // https://github.com/me-no-dev/AsyncTCP [latest]
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
Ticker IthoInitCheck;
Ticker DelayedReq;
Ticker DelayedSave;
Ticker scan;
#if defined (__HW_VERSION_ONE__)
FSInfo fs_info;
#elif defined (__HW_VERSION_TWO__)
Ticker timerLearnLeaveMode;
IthoCC1101 rf;
IthoPacket packet;
IthoRemote remotes;

#define STACK_SIZE_SMALL          2048
#define STACK_SIZE                4096
#define STACK_SIZE_LARGE          8192
#define TASK_MAIN_PRIO            5
#define TASK_CC1101_PRIO          5
#define TASK_MQTT_PRIO            5
#define TASK_WEB_PRIO             5
#define TASK_CONFIG_AND_LOG_PRIO  5
#define TASK_SYS_CONTROL_PRIO     5
static SemaphoreHandle_t mutexLogTask;
static SemaphoreHandle_t mutexJSONLog;
static SemaphoreHandle_t mutexWSsend;
Ticker TaskCC1101Timeout;
Ticker TaskMQTTTimeout;
Ticker TaskConfigAndLogTimeout;
TaskHandle_t xTaskInitHandle = NULL;
TaskHandle_t xTaskCC1101Handle = NULL;
TaskHandle_t xTaskMQTTHandle = NULL;
TaskHandle_t xTaskWebHandle = NULL;
TaskHandle_t xTaskConfigAndLogHandle = NULL;
TaskHandle_t xTaskSysControlHandle = NULL;
uint32_t TaskInitHWmark = 0;
uint32_t TaskCC1101HWmark = 0;
uint32_t TaskMQTTHWmark = 0;
uint32_t TaskWebHWmark = 0;
uint32_t TaskConfigAndLogHWmark = 0;
uint32_t TaskSysControlHWmark = 0;
bool TaskInitReady = false;
StaticTask_t xTaskInitBuffer;
StaticTask_t xTaskCC1101Buffer;
StaticTask_t xTaskMQTTBuffer;
StaticTask_t xTaskWebBuffer;
StaticTask_t xTaskConfigAndLogBuffer;
StaticTask_t xTaskSysControlBuffer;
StackType_t xTaskInitStack[ STACK_SIZE ];
StackType_t xTaskCC1101Stack[ STACK_SIZE ];
StackType_t xTaskMQTTStack[ STACK_SIZE_LARGE ];
StackType_t xTaskWebStack[ STACK_SIZE_LARGE ];
StackType_t xTaskConfigAndLog[ STACK_SIZE_LARGE ];
StackType_t xTaskSysControlStack[ STACK_SIZE_LARGE ];
void TaskInit( void * parameter );
void TaskCC1101( void * parameter );
void TaskMQTT( void * parameter );
void TaskWeb( void * parameter );
void TaskConfigAndLog( void * parameter );
void TaskSysControl( void * parameter );
#endif
IthoQueue ithoQueue;
System sys;
SystemConfig systemConfig;
WifiConfig wifiConfig;

SHTSensor sht_org(SHTSensor::SHT3X);
SHTSensor sht_alt(SHTSensor::SHT3X_ALT);

const char* espName = "nrg-itho-";
const char* WiFiAPPSK = "password"; //default AP mode password

// Global variables
int MQTT_conn_state = -5;
int MQTT_conn_state_new = 0;
unsigned long lastMQTTReconnectAttempt = 0;
unsigned long lastWIFIReconnectAttempt = 0;

volatile uint16_t ithoCurrentVal   = 0;
volatile uint16_t nextIthoVal = 0;
volatile unsigned long nextIthoTimer = 0;

float ithoHum = 0;
float ithoTemp = 0;

#define LOG_BUF_SIZE 128

unsigned long updatetimer = 0;
unsigned long APmodeTimeout = 0;
unsigned long lastSysMessage = 0;
unsigned long previousUpdate = 0;
unsigned long wifiLedUpdate = 0;
unsigned long SHT3x_readout = 0;
unsigned long lastLog = 0;

//flags used
bool coldBoot = false;
bool joinSend = false;
int8_t ithoInit = 0;
bool shouldReboot = false;
bool dontSaveConfig = false;
bool saveSystemConfigflag = false;
bool saveWifiConfigflag = false;
bool resetWifiConfigflag = false;
bool resetSystemConfigflag = false;
bool dontReconnectMQTT = false;
bool clearQueue = false;
bool wifiModeAP = false;
bool sysStatReq = false;
bool runscan = false;
bool updateIthoMQTT = false;
bool SHT3xupdated = false;
volatile bool updateItho = false;
volatile bool ithoCheck = false;
volatile bool saveRemotesflag = false;
bool SHT3x_original = false;
bool SHT3x_alternative = false;
bool rfInitOK = false;

size_t content_len;

typedef enum { WEBINTERFACE, RFLOG } logtype;


void dummyFunct() {
  //some weird stuff with the arduino IDE
}
