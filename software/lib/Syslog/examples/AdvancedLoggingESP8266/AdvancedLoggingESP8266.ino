/*

 Syslog client: AdvancedLogging example

 Demonstrates logging messages to Syslog server in IETF format (rfc5424) in
 combination with the ESP8266 board/library.

 For more about Syslog see https://en.wikipedia.org/wiki/Syslog

 created 3 Nov 2016
 by Martin Sloup

 This code is in the public domain.

 */

#include <ESP8266WiFi.h>
#include <Syslog.h>
#include <WiFiUdp.h>

// WIFI credentials
#define WIFI_SSID "**************"
#define WIFI_PASS "**************"

// Syslog server connection info
#define SYSLOG_SERVER "syslog-server"
#define SYSLOG_PORT 514

// This device info
#define DEVICE_HOSTNAME "my-device"
#define APP_NAME "my-app"
#define ANOTHER_APP_NAME "my-another-app"

// A UDP instance to let us send and receive packets over UDP
WiFiUDP udpClient;

// Create a new empty syslog instance
Syslog syslog(udpClient, SYSLOG_PROTO_IETF);

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
  Serial.println();

  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  // prepare syslog configuration here (can be anywhere before first call of 
	// log/logf method)
  syslog.server(SYSLOG_SERVER, SYSLOG_PORT);
  syslog.deviceHostname(DEVICE_HOSTNAME);
  syslog.appName(APP_NAME);
  syslog.defaultPriority(LOG_KERN);
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

  // You can set default facility and severity level and use it for all log
  // messages (beware defaultPriority is stored permanently!)
  syslog.defaultPriority(LOG_FTP | LOG_INFO);
  syslog.logf("This is ftp info message no. %d", iteration);

  // Return to default facility
  syslog.defaultPriority(LOG_KERN);

  // Also fluent interface is supported (beware appName is stored permanently!)
  syslog.appName(ANOTHER_APP_NAME).logf(LOG_ERR,  
    "This is error message no. %d from %s", iteration, ANOTHER_APP_NAME);

  // Return to default app name
  syslog.appName(APP_NAME);

  // Send log messages only for selected severity levels: LOG_INFO and LOG_WARNING
  syslog.logMask(LOG_MASK(LOG_INFO) | LOG_MASK(LOG_WARNING));
  syslog.log(LOG_INFO, "This is logged.");
  syslog.log(LOG_WARNING, "This is logged.");
  syslog.log(LOG_ERR, "This is not logged.");

  // Send log messages up to LOG_WARNING severity
  syslog.logMask(LOG_UPTO(LOG_WARNING));
  syslog.log(LOG_ERR, "This is logged.");
  syslog.log(LOG_WARNING, "This is logged.");
  syslog.log(LOG_NOTICE, "This is not logged.");
  syslog.log(LOG_INFO, "This is not logged.");
  
  // Return logMask to default
  syslog.logMask(LOG_UPTO(LOG_DEBUG));  

  // F() macro is supported too
  syslog.log(LOG_INFO, F("End loop"));

  iteration++;
  
  // wait ten seconds before sending log message again
  delay(10000);
}
