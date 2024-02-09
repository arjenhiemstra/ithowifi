/*

 Syslog client: AdvancedLogging example

 Demonstrates logging messages to Syslog server in IETF format (rfc5424) in 
 combination with the Ethernet library and Arduino Ethernet Shield.

 For more about Syslog see https://en.wikipedia.org/wiki/Syslog

 created 21 Jan 2017
 by Martin Sloup

 This code is in the public domain.

 */

#include <SPI.h>
#include <Ethernet.h>
#include <EthernetUdp.h>
#include <Syslog.h>

// Syslog server connection info
#define SYSLOG_SERVER "syslog-server"
#define SYSLOG_PORT 514

// This device info
#define DEVICE_HOSTNAME "my-device"
#define APP_NAME "my-app"
#define ANOTHER_APP_NAME "my-another-app"

// Enter a MAC address for your controller below.
// Newer Ethernet shields have a MAC address printed on a sticker on the shield
byte mac[] = {
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED
};

// A UDP instance to let us send and receive packets over UDP
EthernetUDP udpClient;

// Create a new empty syslog instance
Syslog syslog(udpClient, SYSLOG_PROTO_IETF);

int iteration = 1;

void setup() {
  // Open serial communications and wait for port to open:
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  // start Ethernet
  if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");
    // no point in carrying on, so do nothing forevermore:
    for (;;)
      ;
  }

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

  // Send log messages up to LOG_WARNING priority
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
