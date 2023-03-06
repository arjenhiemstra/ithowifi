#include "sys_log.h"

WiFiUDP udpClient;
Syslog syslog(udpClient, SYSLOG_PROTO_IETF);

std::deque<log_msg> syslog_queue;

void printTimestamp(Print *_logOutput, int logLevel)
{
  struct tm timeinfo;
  if (getLocalTime(&timeinfo, 0))
  {
    char timeStringBuff[50]{}; // 50 chars should be enough
    strftime(timeStringBuff, sizeof(timeStringBuff), "%F %T ", &timeinfo);
    _logOutput->print(timeStringBuff);
  }
  else
  {
    char c[32]{};
    snprintf(c, sizeof(c), "%10lu ", millis());
    _logOutput->print(c);
  }
}

void LogPrefixESP(Print *_logOutput, int logLevel)
{
  _logOutput->print("ESP-IDF: ");
}

void printNewline(Print *_logOutput, int logLevel)
{
  _logOutput->print("");
}

void sys_log(log_prio_level_t log_prio, const char *inputString, ...)
{

  log_msg input;
  va_list args;

  va_start(args, inputString);

  size_t len = vsnprintf(0, 0, inputString, args);
  input.msg.resize(len + 1); // need room to NULL terminate the string
  vsnprintf(&input.msg[0], len + 1, inputString, args);
  input.msg.resize(len);

  va_end(args);

#if defined(ENABLE_SERIAL)
  Serial.println(input.msg.c_str());
#endif

  input.code = log_prio;

  if (syslog_queue.size() > SYSLOG_QUEUE_MAX_SIZE)
    return;

  if (syslog_queue.size() == SYSLOG_QUEUE_MAX_SIZE)
  {
    input.code = SYSLOG_WARNING;
    input.msg = "Warning: syslog_queue overflow";
  }

  syslog_queue.push_back(input);

}

int esp_vprintf(const char *fmt, va_list args)
{

  log_msg input;

  size_t len = vsnprintf(0, 0, fmt, args);
  input.msg.resize(len); // NULL terminate the string on the last character, effectively removing carriage return
  vsnprintf(&input.msg[0], len, fmt, args);
  input.msg.resize(len);


#if defined(ENABLE_SERIAL)
  Serial.println(input.msg.c_str());
#endif

  input.code = ESP_SYSLOG_ERR;

  if (syslog_queue.size() > SYSLOG_QUEUE_MAX_SIZE)
    return 0;

  if (syslog_queue.size() == SYSLOG_QUEUE_MAX_SIZE)
  {
    input.code = SYSLOG_WARNING;
    input.msg = "Warning: syslog_queue overflow";
  }

  syslog_queue.push_back(input);

  return 0;//vprintf(fmt, args);
}
