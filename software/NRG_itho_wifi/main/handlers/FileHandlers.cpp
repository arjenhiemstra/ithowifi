#include "tasks/task_web.h"

void handleCoreCrash(AsyncWebServerRequest *request)
{
  request->send(200);
  D_LOG("SYS: Triggering core crash...");
  assert(0);
}

void handleCoredumpDownload(AsyncWebServerRequest *request)
{
  // httpd_resp_set_type(req, "application/octet-stream");
  // httpd_resp_set_hdr(req, "Content-Disposition",
  //                    "attachment;filename=core.bin");

  esp_partition_iterator_t partition_iterator = esp_partition_find(
      ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_COREDUMP, "coredump");

  const esp_partition_t *partition = esp_partition_get(partition_iterator);

  esp_core_dump_summary_t *summary = static_cast<esp_core_dump_summary_t *>(malloc(sizeof(esp_core_dump_summary_t)));

  if (summary)
  {
    esp_err_t err = esp_core_dump_get_summary(summary);
    if (err == ESP_OK)
    {
      D_LOG("SYS: Getting core dump summary ok.");
    }
    else
    {
      D_LOG("SYS: Getting core dump summary not ok. Error: %d\n", (int)err);
      D_LOG("SYS: Probably no coredump present yet.\n");
      D_LOG("SYS: esp_core_dump_image_check() = %s\n", esp_core_dump_image_check() ? "OK" : "NOK");
    }
    free(summary);
  }

  int file_size = 65536;
  int maxLen = 1024;
  char buffer[maxLen];

  AsyncWebServerResponse *response = request->beginChunkedResponse("application/octet-stream", [partition, file_size, &buffer](uint8_t *buffer2, size_t maxLen, size_t alreadySent) -> size_t
                                                                   {
      if (file_size - alreadySent >= maxLen) {
        ESP_ERROR_CHECK(
            esp_partition_read(partition, alreadySent, buffer, maxLen));
        return maxLen;
      } else {
        ESP_ERROR_CHECK(esp_partition_read(partition, alreadySent, buffer,
                                           file_size - alreadySent));
        //memcpy(buffer, fileArray + alreadySent, fileSize - alreadySent);
        return file_size - alreadySent;
      } });

  response->addHeader("Content-Disposition", "attachment; filename=\"coredump.bin\"");
  response->addHeader("Transfer-Encoding", "chunked");
  request->send(response);
}

void handleCurLogDownload(AsyncWebServerRequest *request)
{
  if (systemConfig.syssec_web)
  {
    if (!request->authenticate(systemConfig.sys_username, systemConfig.sys_password))
      return request->requestAuthentication();
  }
  request->send(ACTIVE_FS, getCurrentLogPath(), "", true);
}

void handlePrevLogDownload(AsyncWebServerRequest *request)
{
  if (systemConfig.syssec_web)
  {
    if (!request->authenticate(systemConfig.sys_username, systemConfig.sys_password))
      return request->requestAuthentication();
  }
  request->send(ACTIVE_FS, getPreviousLogPath(), "", true);
}

void handleFileCreate(AsyncWebServerRequest *request)
{
  if (systemConfig.syssec_edit)
  {
    if (!request->authenticate(systemConfig.sys_username, systemConfig.sys_password))
    {
      request->send(401);
      return;
    }
  }

  String path;
  String src;
  int params = request->params();
  for (int i = 0; i < params; i++)
  {
    const AsyncWebParameter *p = request->getParam(i);

    if (strcmp(p->name().c_str(), "path") == 0)
    {
      D_LOG("SYS: handleFileCreate path found ('%s')", p->value().c_str());
      path = p->value().c_str();
    }
    if (strcmp(p->name().c_str(), "src") == 0)
    {
      D_LOG("SYS: handleFileCreate src found ('%s')", p->value().c_str());
      src = p->value().c_str();
    }
    // D_LOG("Param[%d] (name:'%s', value:'%s'", i, p->name().c_str(), p->value().c_str());
  }

  if (path.isEmpty())
  {
    request->send(400, "text/plain", "PATH ARG MISSING");
    return;
  }
  if (path == "/")
  {
    request->send(400, "text/plain", "BAD PATH");
    return;
  }

  if (src.isEmpty())
  {
    // No source specified: creation
    D_LOG("SYS: handleFileCreate: %s", path.c_str());
    if (path.endsWith("/"))
    {
      // Create a folder
      path.remove(path.length() - 1);
      if (!ACTIVE_FS.mkdir(path))
      {
        request->send(500, "text/plain", "MKDIR FAILED");
        return;
      }
    }
    else
    {
      // Create a file
      File file = ACTIVE_FS.open(path, "w");
      if (file)
      {
        file.write(0);
        file.close();
      }
      else
      {
        request->send(500, "text/plain", "CREATE FAILED");
        return;
      }
    }
    request->send(200, "text/plain", path.c_str());
  }
  else
  {
    // Source specified: rename
    if (src == "/")
    {
      request->send(400, "text/plain", "BAD SRC");
      return;
    }
    if (!ACTIVE_FS.exists(src))
    {
      request->send(400, "text/plain", "BSRC FILE NOT FOUND");
      return;
    }

    D_LOG("SYS: handleFileCreate: %s from %s", path.c_str(), src.c_str());
    if (path.endsWith("/"))
    {
      path.remove(path.length() - 1);
    }
    if (src.endsWith("/"))
    {
      src.remove(src.length() - 1);
    }
    if (!ACTIVE_FS.rename(src, path))
    {
      request->send(500, "text/plain", "RENAME FAILED");
      return;
    }
    request->send(200);
  }
}

void handleFileDelete(AsyncWebServerRequest *request)
{
  if (systemConfig.syssec_edit)
  {
    if (!request->authenticate(systemConfig.sys_username, systemConfig.sys_password))
    {
      request->send(401);
      return;
    }
  }
  String path;

  int params = request->params();
  for (int i = 0; i < params; i++)
  {
    const AsyncWebParameter *p = request->getParam(i);

    if (strcmp(p->name().c_str(), "path") == 0)
    {
      path = p->value().c_str();
      D_LOG("SYS: handleFileDelete path found ('%s')", p->value().c_str());
    }
  }

  if (path.isEmpty() || path == "/")
  {
    request->send(500, "text/plain", "BAD PATH");
    return;
  }
  if (!ACTIVE_FS.exists(path))
  {
    request->send(500, "text/plain", "FILE NOT FOUND");
    return;
  }

  File root = ACTIVE_FS.open(path, "r");
  // If it's a plain file, delete it
  if (!root.isDirectory())
  {
    root.close();
    ACTIVE_FS.remove(path);
  }
  else
  {
    ACTIVE_FS.rmdir(path);
  }
  request->send(200);
}

void handleUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final)
{
  if (systemConfig.syssec_edit)
  {
    if (!request->authenticate(systemConfig.sys_username, systemConfig.sys_password))
      return request->requestAuthentication();
  }
  String logmessage = "SYS: Client:" + request->client()->remoteIP().toString() + " " + request->url() + "\n";
  D_LOG(logmessage.c_str());

  if (!index)
  {
    logmessage = "SYS: Upload Start: " + String(filename) + "\n";
    // open the file on first call and store the file handle in the request object
    request->_tempFile = ACTIVE_FS.open("/" + filename, "w");
    D_LOG(logmessage.c_str());
  }

  if (len)
  {
    // stream the incoming chunk to the opened file
    request->_tempFile.write(data, len);
    logmessage = "SYS: Writing file: " + String(filename) + " index=" + String(index) + " len=" + String(len) + "\n";
    D_LOG(logmessage.c_str());
  }

  if (final)
  {
    logmessage = "SYS: Upload Complete: " + String(filename) + ",size: " + String(index + len) + "\n";
    // close the file handle as the upload is now done
    request->_tempFile.close();
    D_LOG(logmessage.c_str());
    request->redirect("/");
  }
}

/*
    Read the given file from the filesystem and stream it back to the client
*/
bool handleFileRead(AsyncWebServerRequest *request)
{
  if (systemConfig.syssec_edit)
  {
    if (!request->authenticate(systemConfig.sys_username, systemConfig.sys_password))
    {
      request->send(401);
      return false;
    }
  }
  String path = request->url();

  // Mask passwords in config files served via file editor
  if (path == "/config.json" || path == "/wifi.json")
  {
    if (ACTIVE_FS.exists(path))
    {
      File file = ACTIVE_FS.open(path, "r");
      if (!file)
        return false;
      JsonDocument doc;
      DeserializationError err = deserializeJson(doc, file);
      file.close();
      if (err)
        return false;
      JsonObject root = doc.as<JsonObject>();
      const char *pwFields[] = {"sys_password", "mqtt_password", "passwd", "appasswd"};
      for (const char *field : pwFields)
      {
        if (!root[field].isNull())
        {
          const char *val = root[field].as<const char *>();
          root[field] = (val && strlen(val) > 0) ? "********" : "";
        }
      }
      String output;
      serializeJson(doc, output);
      request->send(200, "application/json", output);
      return true;
    }
  }

  String pathWithGz = path + ".gz";
  if (ACTIVE_FS.exists(pathWithGz) || ACTIVE_FS.exists(path))
  {
    D_LOG("SYS: file found ('%s')", path.c_str());

    if (ACTIVE_FS.exists(pathWithGz))
    {
      path += ".gz";
    }
    request->send(ACTIVE_FS, path.c_str(), getContentType(request->hasParam("download"), path.c_str()), request->hasParam("download"));

    pathWithGz = String();
    path = String();
    return true;
  }
  pathWithGz = String();
  path = String();
  return false;
}

void handleStatus(AsyncWebServerRequest *request)
{
  if (systemConfig.syssec_edit)
  {
    if (!request->authenticate(systemConfig.sys_username, systemConfig.sys_password))
    {
      request->send(401);
      return;
    }
  }
  size_t totalBytes = ACTIVE_FS.totalBytes();
  size_t usedBytes = ACTIVE_FS.usedBytes();

  String json;
  json.reserve(128);
  json = "{\"type\":\"Filesystem\", \"isOk\":";

  json += "\"true\", \"totalBytes\":\"";
  json += totalBytes;
  json += "\", \"usedBytes\":\"";
  json += usedBytes;
  json += "\"";

  json += ",\"unsupportedFiles\":\"\"}";

  request->send(200, "application/json", json);
  json = String();
}

void handleFileList(AsyncWebServerRequest *request)
{
  if (systemConfig.syssec_edit)
  {
    if (!request->authenticate(systemConfig.sys_username, systemConfig.sys_password))
    {
      request->send(401);
      return;
    }
  }
  if (!request->hasParam("dir"))
  {
    request->send(500, "text/plain", "BAD ARGS\n");
    return;
  }

  String path = request->getParam("dir")->value();
  File root = ACTIVE_FS.open(path);
  path = String();

  String output = "[";
  if (root.isDirectory())
  {
    File file = root.openNextFile();
    while (file)
    {
      if (output != "[")
      {
        output += ',';
      }
      output += "{\"type\":\"";
      output += (file.isDirectory()) ? "dir" : "file";
      output += "\",\"name\":\"";
      output += String(file.path()).substring(1);
      output += "\",\"size\":\"";
      output += String(file.size());
      output += "\"}";
      file = root.openNextFile();
    }
  }
  output += "]";

  root.close();

  request->send(200, "application/json", output);
  output = String();
}
