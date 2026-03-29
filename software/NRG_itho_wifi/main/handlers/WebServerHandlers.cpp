#include "tasks/task_web.h"

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
