#pragma once

#include <WiFi.h>
#include <WiFiClient.h>

class NetworkManager
{
public:
  // Network clients (TLS clients are created per use, a global one holds ~2 KB)
  WiFiClient standardClient;

  NetworkManager();

  void initialize();
  WiFiClient &getStandardClient() { return standardClient; }
};

extern NetworkManager networkManager;
