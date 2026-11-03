// StreamResponse.h
#pragma once

#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <new>
#include <esp_heap_caps.h>

#include "sys_log.h"

// The stream's cbuf throws std::bad_alloc when the heap runs out, which aborts the async_tcp task.
// Build the response with fill(response) and answer 503 instead.
template <typename Fill>
void sendStreamResponse(AsyncWebServerRequest *request, const char *contentType, Fill fill)
{
  AsyncResponseStream *response = nullptr;
  try
  {
    response = request->beginResponseStream(contentType);
    fill(response);
  }
  catch (const std::bad_alloc &)
  {
    delete response;
    E_LOG("NET: heap exhausted, returning 503 (free:%u block:%u)", heap_caps_get_free_size(MALLOC_CAP_8BIT), heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));
    request->send(503, "text/plain", "Service unavailable (low memory)");
    return;
  }
  request->send(response);
}
