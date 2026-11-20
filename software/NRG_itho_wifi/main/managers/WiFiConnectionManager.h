#pragma once

#include <DNSServer.h>

// WiFi state globals (moved from task_syscontrol.cpp)
extern bool wifiModeAP;
extern bool wifiSTAshouldConnect;
extern bool wifiSTAconnected;
extern bool wifiSTAconnecting;
extern unsigned long APmodeTimeout;
extern DNSServer dnsServer;

// WiFi functions (moved from task_syscontrol.h)
void wifiInit();
void WiFiEvent(WiFiEvent_t event, arduino_event_info_t info);
void setupWiFigeneric();
void setupWiFiAP();
bool connectWiFiSTA(bool restore = false);

// Reconnect without blocking the caller: start it once, then poll it from the task loop
enum class WiFiReconnectState : uint8_t
{
  IDLE,
  BUSY,
  CONNECTED,
  FAILED
};
void startWiFiReconnect();
WiFiReconnectState pollWiFiReconnect();
void setStaticIpConfig();
