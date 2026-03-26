#include "api/WebAPIv1.h"
#include "tasks/task_web.h"

void handleAPIv1(AsyncWebServerRequest *request)
{
  bool parseOK = false;

  int params = request->params();

  if (systemConfig.syssec_api)
  {
    bool username = false;
    bool password = false;

    for (int i = 0; i < params; i++)
    {
      const AsyncWebParameter *p = request->getParam(i);
      if (strcmp(p->name().c_str(), "username") == 0)
      {
        if (strcmp(p->value().c_str(), systemConfig.sys_username) == 0)
        {
          username = true;
        }
      }
      else if (strcmp(p->name().c_str(), "password") == 0)
      {
        if (strcmp(p->value().c_str(), systemConfig.sys_password) == 0)
        {
          password = true;
        }
      }
    }
    if (!username || !password)
    {
      request->send(200, "text/html", "AUTHENTICATION FAILED");
      return;
    }
  }

  const char *speed = nullptr;
  const char *timer = nullptr;
  const char *vremotecmd = nullptr;
  const char *vremoteidx = nullptr;
  const char *vremotename = nullptr;
  const char *rfremotecmd = nullptr;
  const char *rfremoteindex = nullptr;

  for (int i = 0; i < params; i++)
  {
    const AsyncWebParameter *p = request->getParam(i);

    if (strcmp(p->name().c_str(), "get") == 0)
    {
      const AsyncWebParameter *q = request->getParam("get");
      if (strcmp(q->value().c_str(), "currentspeed") == 0)
      {
        char ithoval[10]{};
        snprintf(ithoval, sizeof(ithoval), "%d", ithoCurrentVal);
        request->send(200, "text/html", ithoval);
        return;
      }
      else if (strcmp(q->value().c_str(), "queue") == 0)
      {
        AsyncResponseStream *response = request->beginResponseStream("text/html");

        JsonDocument root;
        JsonObject obj = root.to<JsonObject>(); // Fill the object
        JsonArray arr = root["queue"].to<JsonArray>();

        ithoQueue.get(arr);

        obj["ithoSpeed"] = ithoQueue.ithoSpeed;
        obj["ithoOldSpeed"] = ithoQueue.ithoOldSpeed;
        obj["fallBackSpeed"] = ithoQueue.fallBackSpeed;
        serializeJson(root, *response);
        request->send(response);

        return;
      }
      else if (strcmp(q->value().c_str(), "ithostatus") == 0)
      {
        AsyncResponseStream *response = request->beginResponseStream("text/html");
        JsonDocument doc;
        JsonObject root = doc.to<JsonObject>();
        getIthoStatusJSON(root);

        serializeJson(root, *response);
        request->send(response);
        if (doc.overflowed())
        {
          E_LOG("API: error - JsonDocument overflowed (html api)");
        }
        return;
      }
      else if (strcmp(q->value().c_str(), "lastcmd") == 0)
      {
        AsyncResponseStream *response = request->beginResponseStream("text/html");
        JsonDocument doc;

        JsonObject root = doc.to<JsonObject>();
        getLastCMDinfoJSON(root);

        serializeJson(root, *response);
        request->send(response);

        return;
      }
      else if (strcmp(q->value().c_str(), "remotesinfo") == 0)
      {
        AsyncResponseStream *response = request->beginResponseStream("text/html");
        JsonDocument doc;

        JsonObject root = doc.to<JsonObject>();

        getRemotesInfoJSON(root);

        serializeJson(root, *response);
        request->send(response);

        return;
      }
    }
    else if (strcmp(p->name().c_str(), "getsetting") == 0 || strcmp(p->name().c_str(), "setsetting") == 0)
    {
      request->send(200, "text/plain", "Use REST API: GET/PUT /api/v2/settings");
      return;
    }
    else if (strcmp(p->name().c_str(), "command") == 0)
    {
      parseOK = ithoExecCommand(p->value().c_str(), HTMLAPI);
    }
    else if (strcmp(p->name().c_str(), "rfremotecmd") == 0)
    {
      rfremotecmd = p->value().c_str();
      // parseOK = ithoExecRFCommand(p->value().c_str(), HTMLAPI);
    }
    else if (strcmp(p->name().c_str(), "rfremoteindex") == 0)
    {
      rfremoteindex = p->value().c_str();
    }
    else if (strcmp(p->name().c_str(), "vremote") == 0)
    {
      vremotecmd = p->value().c_str();
    }
    else if (strcmp(p->name().c_str(), "vremotecmd") == 0)
    {
      vremotecmd = p->value().c_str();
    }
    else if (strcmp(p->name().c_str(), "vremoteindex") == 0)
    {
      vremoteidx = p->value().c_str();
    }
    else if (strcmp(p->name().c_str(), "vremotename") == 0)
    {
      vremotename = p->value().c_str();
    }
    else if (strcmp(p->name().c_str(), "speed") == 0)
    {
      speed = p->value().c_str();
    }
    else if (strcmp(p->name().c_str(), "timer") == 0)
    {
      timer = p->value().c_str();
    }
    else if (strcmp(p->name().c_str(), "i2csniffer") == 0)
    {
      auto *sg = static_cast<i2c_safe_guard_t *>(i2cManager.safe_guard);
      if (strcmp(p->value().c_str(), "on") == 0)
      {
        i2cSnifferEnable();
        sg->sniffer_enabled = true;
        sg->sniffer_web_enabled = true;
        parseOK = true;
      }
      else if (strcmp(p->value().c_str(), "off") == 0)
      {
        i2cSnifferDisable();
        sg->sniffer_enabled = false;
        sg->sniffer_web_enabled = false;
        parseOK = true;
      }
    }
    else if (strcmp(p->name().c_str(), "debug") == 0)
    {
      if (strcmp(p->value().c_str(), "reboot") == 0)
      {
        shouldReboot = true;
        logMessagejson("Reboot requested", WEBINTERFACE);
        parseOK = true;
      }
    }
  }

  if (vremotecmd != nullptr || vremoteidx != nullptr || vremotename != nullptr)
  {
    if (vremotecmd != nullptr && (vremoteidx == nullptr && vremotename == nullptr))
    {
      ithoI2CCommand(0, vremotecmd, HTMLAPI);

      parseOK = true;
    }
    else
    {
      // exec command for specific vremote
      int index = -1;
      if (vremoteidx != nullptr)
      {
        if (strcmp(vremoteidx, "0") == 0)
        {
          index = 0;
        }
        else
        {
          index = atoi(vremoteidx);
          if (index == 0)
            index = -1;
        }
      }
      if (vremotename != nullptr)
      {
        index = virtualRemotes.getRemoteIndexbyName(vremotename);
      }
      if (index == -1)
        parseOK = false;
      else
      {
        ithoI2CCommand(index, vremotecmd, HTMLAPI);

        parseOK = true;
      }
    }
  }
  else if (speed != nullptr || timer != nullptr)
  {
    // check speed & timer
    if (speed != nullptr && timer != nullptr)
    {
      parseOK = ithoSetSpeedTimer(speed, timer, HTMLAPI);
    }
    else if (speed != nullptr && timer == nullptr)
    {
      parseOK = ithoSetSpeed(speed, HTMLAPI);
    }
    else if (timer != nullptr && speed == nullptr)
    {
      parseOK = ithoSetTimer(timer, HTMLAPI);
    }
  }
  else if (rfremotecmd != nullptr || rfremoteindex != nullptr)
  {
    uint8_t idx = 0;
    if (rfremoteindex != nullptr)
    {
      idx = strtoul(rfremoteindex, NULL, 10);
    }
    if (rfremotecmd != nullptr)
    {
      parseOK = ithoExecRFCommand(idx, rfremotecmd, HTMLAPI);
    }
  }
  if (parseOK)
  {
    request->send(200, "text/html", "OK");
  }
  else
  {
    request->send(200, "text/html", "NOK");
  }
}
