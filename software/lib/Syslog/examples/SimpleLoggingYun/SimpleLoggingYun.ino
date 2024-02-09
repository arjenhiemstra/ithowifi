/*

 Syslog client: SimpleLogging example

 Demonstrates logging messages to Syslog server in IETF format (rfc5424) in 
 combination with the Bridge and BridgeUDP library on Arduino Yún.

 For more about Syslog see https://en.wikipedia.org/wiki/Syslog

 created 22 Jan 2017
 by Martin Sloup

 This code is in the public domain.

 */

#include <Bridge.h>
#include <BridgeUdp.h>
#include <Syslog.h>

// Syslog server connection info
#define SYSLOG_SERVER "syslog-server"
#define SYSLOG_PORT 514

// This device info
#define DEVICE_HOSTNAME "my-device"
#define APP_NAME "my-app"


// A UDP instance to let us send and receive packets over UDP
BridgeUDP udpClient;

// Create a new syslog instance with LOG_KERN facility
Syslog syslog(udpClient, SYSLOG_SERVER, SYSLOG_PORT, DEVICE_HOSTNAME, APP_NAME, LOG_KERN);

int iteration = 1;

void setup() {
  // Bridge takes about two seconds to start up
  // it can be helpful to use the on-board LED
  // as an indicator for when it has initialized
  pinMode(13, OUTPUT);
  digitalWrite(13, LOW);
  Bridge.begin();
  digitalWrite(13, HIGH);
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