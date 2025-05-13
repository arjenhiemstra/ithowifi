// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright 2016-2025 Hristo Gochkov, Mathieu Carbou, Emil Muratov

//
// SSE example
//

#include <Arduino.h>
#include <AsyncTCP.h>
#include <WiFi.h>

#include <ESPAsyncWebServer.h>

static const char *htmlContent PROGMEM = R"(
<!DOCTYPE html>
<html>
<head>
  <title>Server-Sent Events</title>
  <script>
    if (!!window.EventSource) {
      var source = new EventSource('/events');
      source.addEventListener('open', function(e) {
        console.log("Events Connected");
      }, false);
      source.addEventListener('error', function(e) {
        if (e.target.readyState != EventSource.OPEN) {
          console.log("Events Disconnected");
        }
      }, false);
      source.addEventListener('message', function(e) {
        console.log("message", e.data);
      }, false);
      source.addEventListener('heartbeat', function(e) {
        console.log("heartbeat", e.data);
      }, false);
    }
  </script>
</head>
<body>
  <h1>Open your browser console!</h1>
</body>
</html>
)";

static const size_t htmlContentLength = strlen_P(htmlContent);

static AsyncWebServer server(80);
static AsyncEventSource events("/events");

void setup() {
  Serial.begin(115200);

#ifndef CONFIG_IDF_TARGET_ESP32H2
  WiFi.mode(WIFI_AP);
  WiFi.softAP("esp-captive");
#endif

  // curl -v http://192.168.4.1/
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    // need to cast to uint8_t*
    // if you do not, the const char* will be copied in a temporary String buffer
    request->send(200, "text/html", (uint8_t *)htmlContent, htmlContentLength);
  });

  events.onConnect([](AsyncEventSourceClient *client) {
    Serial.printf("SSE Client connected! ID: %" PRIu32 "\n", client->lastId());
    client->send("hello!", NULL, millis(), 1000);
  });

  events.onDisconnect([](AsyncEventSourceClient *client) {
    Serial.printf("SSE Client disconnected! ID: %" PRIu32 "\n", client->lastId());
  });

  server.addHandler(&events);

  server.begin();
}

static uint32_t lastSSE = 0;
static uint32_t deltaSSE = 3000;

static uint32_t lastHeap = 0;

void loop() {
  uint32_t now = millis();
  if (now - lastSSE >= deltaSSE) {
    events.send(String("ping-") + now, "heartbeat", now);
    lastSSE = millis();
  }

  if (now - lastHeap >= 2000) {
    Serial.printf("Free heap: %" PRIu32 "\n", ESP.getFreeHeap());
    lastHeap = now;
  }
}
