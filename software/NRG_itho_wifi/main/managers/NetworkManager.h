#pragma once

#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>

class NetworkManager
{
public:
  // Network clients
  WiFiClientSecure secureClient;
  WiFiClient standardClient;

  NetworkManager();

  void initialize();
  WiFiClientSecure &getSecureClient() { return secureClient; }
  WiFiClient &getStandardClient() { return standardClient; }
};

extern NetworkManager networkManager;
