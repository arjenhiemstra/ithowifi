#if defined(ARDUINO_ARCH_ESP8266)
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#elif defined(ARDUINO_ARCH_ESP32)
#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#else
#error Unsuported platform
#endif

#include <StreamUtils.h>

// WiFi network configuration.
const char* WIFI_SSID = "*****EDIT*****";
const char* WIFI_PASSWORD = "*****EDIT*****";

void setup() {
  // Initialize Serial Port
  Serial.begin(115200);
  while (!Serial)
    continue;

  // Connect to the WLAN
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.println(F("Connecting to Wifi..."));
    delay(500);
  }

  // Initialize the SSL library
  WiFiClientSecure client;
  client.setInsecure();  // ignore server's certificate

  // Send the request
  HTTPClient http;
  http.begin(client, F("https://jigsaw.w3.org/HTTP/ChunkedScript"));

  // Ask HTTPClient to collect the Transfer-Encoding header
  // (by default it discards all headers)
  const char* keys[] = {"Transfer-Encoding"};
  http.collectHeaders(keys, 1);

  Serial.println(F("Sending request..."));
  int status = http.GET();
  if (status != 200) {
    Serial.print(F("Unexpected HTTP status: "));
    Serial.println(status);
    return;
  }

  // Get a reference to the stream
  Stream& rawStream = http.getStream();
  ChunkDecodingStream decodedStream(http.getStream());
  Stream& response =
      http.header("Transfer-Encoding") == "chunked" ? decodedStream : rawStream;

  // Read and print the response
  char buffer[256];
  size_t n = 0;
  do {
    n = response.readBytes(buffer, sizeof(buffer));
    Serial.write(buffer, n);
  } while (n == sizeof(buffer));

  // Disconnect
  http.end();

  Serial.println(F("Done!"));
}

void loop() {}
