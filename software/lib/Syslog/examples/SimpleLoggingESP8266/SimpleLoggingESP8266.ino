/*

 Syslog client: SimpleLogging example

 Demonstrates logging messages to Syslog server in IETF format (rfc5424) in 
 combination with the ESP8266 board/library.
 
 For more about Syslog see https://en.wikipedia.org/wiki/Syslog

 created 3 Nov 2016
 by Martin Sloup

 This code is in the public domain.

 */

#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <Syslog.h>

// WIFI credentials
#define WIFI_SSID "**************"
#define WIFI_PASS "**************"

// Syslog server connection info
#define SYSLOG_SERVER "syslog-server"
#define SYSLOG_PORT 514

// This device info
#define DEVICE_HOSTNAME "my-device"
#define APP_NAME "my-app"

// A UDP instance to let us send and receive packets over UDP
WiFiUDP udpClient;

// Create a new syslog instance with LOG_KERN facility
Syslog syslog(udpClient, SYSLOG_SERVER, SYSLOG_PORT, DEVICE_HOSTNAME, APP_NAME, LOG_KERN);
int iteration = 1;

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println();

  // We start by connecting to a WiFi network
  Serial.print("Connecting to ");
  Serial.println(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");

  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  // Severity levels can be found in Syslog.h. They are same like in Linux 
  // syslog.
  syslog.log(LOG_INFO, "Begin loop");

  // Log message can be formated like with printf function.
  syslog.logf(LOG_ERR,  "This is error message no. %d", iteration);
  syslog.logf(LOG_INFO, "This is info message no. %d", iteration);

  // You can force set facility in pri parameter for this log message. More 
  // facilities in syslog.h or in Linux syslog documentation.
  syslog.logf(LOG_DAEMON | LOG_INFO, "This is daemon info message no. %d", 
    iteration);

  // F() macro is supported too
  syslog.log(LOG_INFO, F("End loop"));
  iteration++;
  
  // wait ten seconds before sending log message again
  delay(10000);
}
