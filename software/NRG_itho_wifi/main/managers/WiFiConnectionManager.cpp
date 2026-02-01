#include "tasks/task_syscontrol.h"

DNSServer dnsServer;
bool wifiModeAP = false;
bool wifiSTAshouldConnect = false;
bool wifiSTAconnected = false;
bool wifiSTAconnecting = false;
unsigned long APmodeTimeout = 0;

void wifiInit()
{
  WifiConfigLoaded = loadWifiConfig("flash");
  if (!WifiConfigLoaded)
  {
    E_LOG("SYS: Wifi config load failed");
  }

  WiFi.onEvent(WiFiEvent);
  setupWiFigeneric();

  if (wifiConfig.aptimeout > 0)
  {
    I_LOG("SYS: starting wifi access point");
    setupWiFiAP();
  }
  if (strcmp(wifiConfig.ssid, "") != 0)
  {
    connectWiFiSTA();
  }
  else
  {
    I_LOG("SYS: initial wifi config still there");
  }

  if (strcmp(wifiConfig.ntpserver, "") != 0)
  {
    configTime(0, 0, wifiConfig.ntpserver);
  }

  // set timezone

  const char *tz_string = wifiConfig.getTimeZoneStr();

  N_LOG("SYS: timezone: %s, specifier %s ", wifiConfig.timezone, tz_string);
  setenv("TZ", tz_string, 1);
  tzset();

  WiFi.scanDelete();
  if (WiFi.scanComplete() == -2)
  {
    WiFi.scanNetworks(true);
  }
}

void WiFiEvent(WiFiEvent_t event, arduino_event_info_t info)
{
  /*
  * WiFi Events

  0  ARDUINO_EVENT_WIFI_READY               < ESP32 WiFi ready
  1  ARDUINO_EVENT_WIFI_SCAN_DONE                < ESP32 finish scanning AP
  2  ARDUINO_EVENT_WIFI_STA_START                < ESP32 station start
  3  ARDUINO_EVENT_WIFI_STA_STOP                 < ESP32 station stop
  4  ARDUINO_EVENT_WIFI_STA_CONNECTED            < ESP32 station connected to AP
  5  ARDUINO_EVENT_WIFI_STA_DISCONNECTED         < ESP32 station disconnected from AP
  6  ARDUINO_EVENT_WIFI_STA_AUTHMODE_CHANGE      < the auth mode of AP connected by ESP32 station changed
  7  ARDUINO_EVENT_WIFI_STA_GOT_IP               < ESP32 station got IP from connected AP
  8  ARDUINO_EVENT_WIFI_STA_LOST_IP              < ESP32 station lost IP and the IP is reset to 0
  9  ARDUINO_EVENT_WPS_ER_SUCCESS       < ESP32 station wps succeeds in enrollee mode
  10 ARDUINO_EVENT_WPS_ER_FAILED        < ESP32 station wps fails in enrollee mode
  11 ARDUINO_EVENT_WPS_ER_TIMEOUT       < ESP32 station wps timeout in enrollee mode
  12 ARDUINO_EVENT_WPS_ER_PIN           < ESP32 station wps pin code in enrollee mode
  13 ARDUINO_EVENT_WIFI_AP_START                 < ESP32 soft-AP start
  14 ARDUINO_EVENT_WIFI_AP_STOP                  < ESP32 soft-AP stop
  15 ARDUINO_EVENT_WIFI_AP_STACONNECTED          < a station connected to ESP32 soft-AP
  16 ARDUINO_EVENT_WIFI_AP_STADISCONNECTED       < a station disconnected from ESP32 soft-AP
  17 ARDUINO_EVENT_WIFI_AP_STAIPASSIGNED         < ESP32 soft-AP assign an IP to a connected station
  18 ARDUINO_EVENT_WIFI_AP_PROBEREQRECVED        < Receive probe request packet in soft-AP interface
  19 ARDUINO_EVENT_WIFI_AP_GOT_IP6               < ESP32 ap interface v6IP addr is preferred
  19 ARDUINO_EVENT_WIFI_STA_GOT_IP6              < ESP32 station interface v6IP addr is preferred
  20 ARDUINO_EVENT_ETH_START                < ESP32 ethernet start
  21 ARDUINO_EVENT_ETH_STOP                 < ESP32 ethernet stop
  22 ARDUINO_EVENT_ETH_CONNECTED            < ESP32 ethernet phy link up
  23 ARDUINO_EVENT_ETH_DISCONNECTED         < ESP32 ethernet phy link down
  24 ARDUINO_EVENT_ETH_GOT_IP               < ESP32 ethernet got IP from connected AP
  19 ARDUINO_EVENT_ETH_GOT_IP6              < ESP32 ethernet interface v6IP addr is preferred
  25 ARDUINO_EVENT_MAX
  */
  char buf[70]{};

  switch (event)
  {
  case ARDUINO_EVENT_WIFI_READY:
    strlcpy(buf, "WiFi interface ready", sizeof(buf));
    break;
  case ARDUINO_EVENT_WIFI_SCAN_DONE:
    strlcpy(buf, "Completed scan for access points", sizeof(buf));
    break;
  case ARDUINO_EVENT_WIFI_STA_START:
    strlcpy(buf, "WiFi client started", sizeof(buf));
    break;
  case ARDUINO_EVENT_WIFI_STA_STOP:
    wifiSTAshouldConnect = false;
    strlcpy(buf, "WiFi clients stopped", sizeof(buf));
    break;
  case ARDUINO_EVENT_WIFI_STA_CONNECTED:
    strlcpy(buf, "Connected to access point", sizeof(buf));
    break;
  case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
    wifiSTAconnected = false;
    wifiSTAshouldConnect = true;
    strlcpy(buf, "Disconnected from WiFi access point", sizeof(buf));
    break;
  case ARDUINO_EVENT_WIFI_STA_AUTHMODE_CHANGE:
    strlcpy(buf, "Authentication mode of access point has changed", sizeof(buf));
    break;
  case ARDUINO_EVENT_WIFI_STA_GOT_IP:
  {
    wifiSTAconnected = true;
    logWifiInfo();
    strlcpy(buf, "WiFi client got IP", sizeof(buf));
    break;
  }
  case ARDUINO_EVENT_WIFI_STA_LOST_IP:
    wifiSTAconnected = false;
    wifiSTAshouldConnect = true;
    strlcpy(buf, "Lost IP address and IP address is reset to 0", sizeof(buf));
    break;
  case ARDUINO_EVENT_WPS_ER_SUCCESS:
    strlcpy(buf, "WiFi Protected Setup (WPS): succeeded in enrollee mode", sizeof(buf));
    break;
  case ARDUINO_EVENT_WPS_ER_FAILED:
    strlcpy(buf, "WiFi Protected Setup (WPS): failed in enrollee mode", sizeof(buf));
    break;
  case ARDUINO_EVENT_WPS_ER_TIMEOUT:
    strlcpy(buf, "WiFi Protected Setup (WPS): timeout in enrollee mode", sizeof(buf));
    break;
  case ARDUINO_EVENT_WPS_ER_PIN:
    strlcpy(buf, "WiFi Protected Setup (WPS): pin code in enrollee mode", sizeof(buf));
    break;
  case ARDUINO_EVENT_WIFI_AP_START:
  {
    wifiModeAP = true;
    strlcpy(buf, "WiFi access point started", sizeof(buf));
    break;
  }
  case ARDUINO_EVENT_WIFI_AP_STOP:
  {
    wifiModeAP = false;
    strlcpy(buf, "WiFi access point stopped", sizeof(buf));
    break;
  }
  case ARDUINO_EVENT_WIFI_AP_STACONNECTED:
    strlcpy(buf, "Client connected", sizeof(buf));
    break;
  case ARDUINO_EVENT_WIFI_AP_STADISCONNECTED:
    strlcpy(buf, "Client disconnected", sizeof(buf));
    break;
  case ARDUINO_EVENT_WIFI_AP_STAIPASSIGNED:
    strlcpy(buf, "Assigned IP address to client", sizeof(buf));
    break;
  case ARDUINO_EVENT_WIFI_AP_PROBEREQRECVED:
    strlcpy(buf, "Received probe request", sizeof(buf));
    break;
  case ARDUINO_EVENT_WIFI_AP_GOT_IP6:
    strlcpy(buf, "AP IPv6 is preferred", sizeof(buf));
    break;
  case ARDUINO_EVENT_WIFI_STA_GOT_IP6:
    strlcpy(buf, "STA IPv6 is preferred", sizeof(buf));
    break;
  case ARDUINO_EVENT_ETH_GOT_IP6:
    break;
  case ARDUINO_EVENT_ETH_START:
    break;
  case ARDUINO_EVENT_ETH_STOP:
    break;
  case ARDUINO_EVENT_ETH_CONNECTED:
    break;
  case ARDUINO_EVENT_ETH_DISCONNECTED:
    break;
  case ARDUINO_EVENT_ETH_GOT_IP:
    break;
  default:
    break;
  }

  D_LOG("WiFi-event: %d - %s", event, buf);
}

void setupWiFiAP()
{

  static char apname[32]{};

  snprintf(apname, sizeof(apname), "%s%02x%02x", espName, sys.getMac(4), sys.getMac(5));

  /* Soft AP network parameters */
  IPAddress apIP(192, 168, 4, 1);
  IPAddress netMsk(255, 255, 255, 0);

  WiFi.softAPConfig(apIP, apIP, netMsk);
  WiFi.softAP(apname, wifiConfig.appasswd);
  WiFi.enableAP(true);

  delay(500);

  /* Setup the DNS server redirecting all the domains to the apIP */
  dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
  dnsServer.start(53, "*", apIP);

  // wifiModeAP = true;

  APmodeTimeout = millis();
  N_LOG("NET: wifi AP mode started");
}

void setupWiFigeneric()
{

  // Do not use SDK storage of SSID/WPA parameters
  WiFi.persistent(false);

  // Set wifi mode to STA for next step (clear config)
  if (!WiFi.mode(WIFI_STA))
    E_LOG("NET: Unable to set WiFi mode to STA");

  // Clear any saved wifi config
  esp_err_t wifi_restore = esp_wifi_restore();
  D_LOG("NET: esp_wifi_restore: %s", esp_err_to_name(wifi_restore));

  // Disconnect any existing connections and clear
  if (!WiFi.disconnect(true, true))
    W_LOG("NET: unable to set wifi disconnect");

  // Reset wifi mode to NULL
  if (!WiFi.mode(WIFI_MODE_NULL))
    W_LOG("NET: unable to set WiFi mode to NULL");

  // Set hostname
  bool setHostname_result = WiFi.setHostname(hostName());
  D_LOG("NET: WiFi.setHostname: %s", setHostname_result ? "OK" : "NOK");

  // No AutoReconnect
  if (!WiFi.setAutoReconnect(false))
    W_LOG("NET: unable to set auto reconnect");

  delay(200);

  // Begin init of actual wifi connection

  // Set correct mode
  if (wifiConfig.aptimeout > 0)
  {
    if (WiFi.mode(WIFI_AP_STA))
      I_LOG("NET: WiFi mode WIFI_AP_STA");
  }
  else
  {
    if (WiFi.mode(WIFI_STA))
      I_LOG("NET: WiFi mode WIFI_STA");
  }

  esp_err_t esp_wifi_set_max_tx_power(int8_t power);
  esp_err_t wifi_set_max_tx_power = esp_wifi_set_max_tx_power(78);
  D_LOG("NET: esp_wifi_set_max_tx_power: %s", esp_err_to_name(wifi_set_max_tx_power));

  int8_t wifi_power_level = -1;
  esp_err_t wifi_get_max_tx_power = esp_wifi_get_max_tx_power(&wifi_power_level);
  D_LOG("NET: esp_wifi_get_max_tx_power: %s - level:%d", esp_err_to_name(wifi_get_max_tx_power), wifi_power_level);

  // Do not use flash storage for wifi settings
  esp_err_t wifi_set_storage = esp_wifi_set_storage(WIFI_STORAGE_RAM);
  D_LOG("NET: esp_wifi_set_storage: %s", esp_err_to_name(wifi_set_storage));

  // No power saving
  esp_err_t wifi_set_ps = esp_wifi_set_ps(WIFI_PS_NONE);
  D_LOG("NET: esp_wifi_set_ps: %s", esp_err_to_name(wifi_set_ps));
}

bool connectWiFiSTA(bool restore)
{
  wifiSTAconnecting = true;
  // Switch from defualt WIFI_FAST_SCAN ScanMethod to WIFI_ALL_CHANNEL_SCAN. Also set sortMethod to WIFI_CONNECT_AP_BY_SIGNAL just to be sure.
  // WIFI_FAST_SCAN will connect to the first SSID match found causing connection issues when multiple AP active with the same SSID name
  WiFi.setSortMethod(WIFI_CONNECT_AP_BY_SIGNAL);
  WiFi.setScanMethod(WIFI_ALL_CHANNEL_SCAN);

  if (strcmp(wifiConfig.dhcp, "off") == 0)
  {
    setStaticIpConfig();
  }

  delay(2000);

  // wifiModeAP = false;
  D_LOG("NET: Connecting to wireless network...");

  WiFi.begin(wifiConfig.ssid, wifiConfig.passwd);

  auto timeoutmillis = millis() + 30000;
  wl_status_t status = WiFi.status();

  while (millis() < timeoutmillis)
  {

    esp_task_wdt_reset();

    status = WiFi.status();
    if (status == WL_CONNECTED)
    {
      digitalWrite(hardwareManager.wifi_led_pin, LOW);
      wifiSTAconnecting = false;
      return true;
    }
    if (digitalRead(hardwareManager.wifi_led_pin) == LOW)
    {
      digitalWrite(hardwareManager.wifi_led_pin, HIGH);
    }
    else
    {
      digitalWrite(hardwareManager.wifi_led_pin, LOW);
    }

    delay(100);
  }

  digitalWrite(hardwareManager.wifi_led_pin, HIGH);

  E_LOG("NET: error - could not connect to wifi - %s", wifiConfig.wl_status_to_name(status));
  wifiSTAconnecting = false;
  wifiSTAshouldConnect = true;
  return false;
}

void setStaticIpConfig()
{
  bool configOK = true;
  IPAddress staticIP;
  IPAddress gateway;
  IPAddress subnet;
  IPAddress dns1;
  IPAddress dns2;

  if (!staticIP.fromString(wifiConfig.ip))
  {
    configOK = false;
  }
  if (!gateway.fromString(wifiConfig.gateway))
  {
    configOK = false;
  }
  if (!subnet.fromString(wifiConfig.subnet))
  {
    configOK = false;
  }
  if (!dns1.fromString(wifiConfig.dns1))
  {
    configOK = false;
  }
  if (!dns2.fromString(wifiConfig.dns2))
  {
    configOK = false;
  }
  if (configOK)
  {
    if (!WiFi.config(staticIP, gateway, subnet, dns1, dns2))
    {
      E_LOG("NET: error - static IP config NOK");
    }
    else
    {
      I_LOG("NET: static IP config OK");
    }
  }
}
