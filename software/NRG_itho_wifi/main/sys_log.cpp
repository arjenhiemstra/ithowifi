#include "sys_log.h"

WiFiUDP udpClient;
Syslog syslog(udpClient, SYSLOG_PROTO_IETF);

std::deque<log_msg> syslog_queue;
SemaphoreHandle_t syslog_queueSemaphore = NULL;

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

// Caller holds syslog_queueSemaphore
static void syslogPush(log_msg &input)
{
  if (syslog_queue.size() > SYSLOG_QUEUE_MAX_SIZE)
    return;

  if (syslog_queue.size() == SYSLOG_QUEUE_MAX_SIZE)
  {
    input.code = SYSLOG_WARNING;
    input.msg = "Warning: syslog_queue overflow";
  }

  syslog_queue.push_back(std::move(input));
}

static void syslogFormat(log_msg &input, const char *fmt, va_list args)
{
  va_list args2;
  va_copy(args2, args);
  size_t len = vsnprintf(0, 0, fmt, args2);
  va_end(args2);
  input.msg.resize(len + 1); // need room to NULL terminate the string
  vsnprintf(&input.msg[0], len + 1, fmt, args);
  input.msg.resize(len);
}

void sys_log(log_prio_level_t log_prio, const char *inputString, ...)
{
  log_msg input;
  va_list args;
  va_start(args, inputString);
  syslogFormat(input, inputString, args);
  va_end(args);

#if defined(ENABLE_SERIAL)
  Serial.println(input.msg.c_str());
#endif

  input.code = log_prio;

  if (xSemaphoreTakeFromISR(syslog_queueSemaphore, NULL) == pdTRUE)
  {
    syslogPush(input);
    xSemaphoreGiveFromISR(syslog_queueSemaphore, NULL);
  }
}

// Takes the oldest message off the queue; false when it is empty or busy
bool syslogPop(log_msg &output)
{
  if (xSemaphoreTake(syslog_queueSemaphore, pdMS_TO_TICKS(10)) != pdTRUE)
    return false;
  bool popped = !syslog_queue.empty();
  if (popped)
  {
    output = std::move(syslog_queue.front());
    syslog_queue.pop_front();
  }
  xSemaphoreGive(syslog_queueSemaphore);
  return popped;
}

int esp_vprintf(const char *fmt, va_list args)
{
  log_msg input;
  syslogFormat(input, fmt, args);
  if (!input.msg.empty() && input.msg.back() == '\n')
    input.msg.pop_back(); // drop the trailing newline

#if defined(ENABLE_SERIAL)
  Serial.println(input.msg.c_str());
#endif

  input.code = ESP_SYSLOG_ERR;

  if (syslog_queueSemaphore != NULL && xSemaphoreTakeFromISR(syslog_queueSemaphore, NULL) == pdTRUE)
  {
    syslogPush(input);
    xSemaphoreGiveFromISR(syslog_queueSemaphore, NULL);
  }

  return 0; // vprintf(fmt, args);
}
