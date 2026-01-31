#include "tasks/task_web.h"

void ArduinoOTAinit()
{
  // Send OTA events to the browser
  ArduinoOTA.setHostname(hostName());
  ArduinoOTA
      .onStart([]()
               {
                 static char buf[128]{};
                 strcat(buf, "SYS: firmware update: ");

                 if (ArduinoOTA.getCommand() == U_FLASH)
                   strncat(buf, "sketch", sizeof(buf) - strlen(buf) - 1);
                 else // U_SPIFFS
                   strncat(buf, "filesystem", sizeof(buf) - strlen(buf) - 1);
                 // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
                 N_LOG(buf);
                 delay(250);

                 ACTIVE_FS.end();
                 onOTA = true;
                 dontReconnectMQTT = true;
                 mqttClient.disconnect();

                 if (xTaskCC1101Handle != NULL)
                 {
                   disableRF_ISR();
                   TaskCC1101Timeout.detach();
                   vTaskSuspend(xTaskCC1101Handle);
                   esp_task_wdt_delete(xTaskCC1101Handle);
                 }
                 if (xTaskMQTTHandle != NULL)
                 {
                   TaskMQTTTimeout.detach();
                   vTaskSuspend(xTaskMQTTHandle);
                   esp_task_wdt_delete(xTaskMQTTHandle);
                 }
                 if (xTaskConfigAndLogHandle != NULL)
                 {
                   TaskConfigAndLogTimeout.detach();
                   vTaskSuspend(xTaskConfigAndLogHandle);
                   esp_task_wdt_delete(xTaskConfigAndLogHandle);
                 } })
      .onEnd([]()
             { D_LOG("\nOTA: End"); })
      .onProgress([](unsigned int progress, unsigned int total)
                  { D_LOG("POTA: rogress: %u%%\r", (progress / (total / 100))); })
      .onError([](ota_error_t error)
               {
    D_LOG("OTA: Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) D_LOG("OTA: Auth Failed");
    else if (error == OTA_BEGIN_ERROR) D_LOG("OTA: Begin Failed");
    else if (error == OTA_CONNECT_ERROR) D_LOG("OTA: Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) D_LOG("OTA: Receive Failed");
    else if (error == OTA_END_ERROR)
      D_LOG("OTA: End Failed"); });
  ArduinoOTA.begin();
}

void MDNSinit()
{
  mdns_free();
  // initialize mDNS service
  esp_err_t err = mdns_init();
  if (err)
  {
    E_LOG("NET: mDNS init failed: %d", err);
    return;
  }

  char hostname[32]{};
  snprintf(hostname, sizeof(hostname), "%s", hostName());
  for (int i = 0; i < strlen(hostname); i++)
  {
    hostname[i] = tolower(hostname[i]);
  }

  // set hostname
  mdns_hostname_set(hostname);

  char inst_name[sizeof(hostname) + 14]{}; // " Web Interface" == 14 chars
  snprintf(inst_name, sizeof(inst_name), "%s Web Interface", hostname);

  // set default instance
  mdns_instance_name_set(inst_name);

  // add our services
  mdns_service_add(NULL, "_http", "_tcp", 80, NULL, 0);
  mdns_service_add(NULL, "_websocket", "_tcp", 8000, NULL, 0);

  mdns_service_instance_name_set("_http", "_tcp", inst_name);

  N_LOG("NET: mDNS started");

  N_LOG("NET: hostname - %s", hostName());
}

bool prevlogAvailable()
{
  if (ACTIVE_FS.exists("/logfile0.log") || ACTIVE_FS.exists("/logfile1.log"))
    return true;

  return false;
}

const char *getCurrentLogPath()
{
  return ACTIVE_FS.exists("/logfile0.current.log") ? "/logfile0.current.log" : "/logfile1.current.log";
}

const char *getPreviousLogPath()
{
  return ACTIVE_FS.exists("/logfile0.current.log") ? "/logfile1.log" : "/logfile0.log";
}

const char *getContentType(bool download, const char *filename)
{
  if (download)
    return "application/octet-stream";
  else if (strstr(filename, ".htm"))
    return "text/html";
  else if (strstr(filename, ".html"))
    return "text/html";
  else if (strstr(filename, ".log"))
    return "text/plain";
  else if (strstr(filename, ".css"))
    return "text/css";
  else if (strstr(filename, ".sass"))
    return "text/css";
  else if (strstr(filename, ".js"))
    return "application/javascript";
  else if (strstr(filename, ".png"))
    return "image/png";
  else if (strstr(filename, ".svg"))
    return "image/svg+xml";
  else if (strstr(filename, ".gif"))
    return "image/gif";
  else if (strstr(filename, ".jpg"))
    return "image/jpeg";
  else if (strstr(filename, ".ico"))
    return "image/x-icon";
  else if (strstr(filename, ".xml"))
    return "text/xml";
  else if (strstr(filename, ".pdf"))
    return "application/x-pdf";
  else if (strstr(filename, ".zip"))
    return "application/x-zip";
  else if (strstr(filename, ".gz"))
    return "application/x-gzip";
  return "text/plain";
}
