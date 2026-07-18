# ADR-0006: REST API v2 Logic/Routing Split

Status: Accepted

## Context

The REST v2 API needs to be testable on a host machine (no ESP32 hardware) while still binding to `ESPAsyncWebServer` for actual HTTP serving. Evidence: `main/api/WebAPIv2.h`/`.cpp` contains framework-agnostic `processXxx` functions (`processGetCommands`, `processSetsettingCommands`, `processCommand`, `processSetRFremote`, `processRFCO2Command`, etc. — `WebAPIv2.h:7-19`) that take/return JSON via `ApiResponse`, with no dependency on `AsyncWebServer`. `main/api/WebAPIv2Rest.cpp` contains the route bindings (`handleGetXxx`/`handlePostXxx`) that parse the `AsyncWebServerRequest`, call the matching `processXxx`, and write the HTTP response. `software/NRG_itho_wifi/test/test_native_api_validation` tests the `processXxx` validation logic directly on the `native_test` PlatformIO environment, without linking ESP32/network code — this is only possible because the split exists. `AsyncWebServerAdapter.h`/`IWebServerHandler.h` further abstract the web server behind an interface.

## Decision

Keep all REST v2 business logic in framework-agnostic `processXxx` functions returning `api_response_status_t`/JSON via `ApiResponse`; keep all `AsyncWebServer`-specific request/response handling in separate `handleXxx` functions that call into the logic layer.

## Consequences

**Positive**: business logic is unit-testable on `native_test` without hardware or a running web server; the same logic layer is reused by MQTT (`MqttAPI.cpp`, see [ADR-0005](ADR-0005-mqtt-integration-design.md)), avoiding duplicated validation.

**Negative**: adds one layer of indirection per endpoint (route handler → process function) that must be kept in sync — a `processXxx` signature change requires updating its `handleXxx` caller too.

## Alternatives Considered

- **Inline logic directly in `AsyncWebServer` route lambdas**: would be simpler per-endpoint but untestable without hardware/network stack, and would duplicate validation logic between the REST and MQTT command paths. Not the pattern used anywhere in `WebAPIv2Rest.cpp` today.

## Related Components

`main/api/WebAPIv2.h`, `main/api/WebAPIv2.cpp`, `main/api/WebAPIv2Rest.cpp`, `main/api/ApiResponse.h`, `main/api/AsyncWebServerAdapter.h`, `main/api/IWebServerHandler.h`, `software/NRG_itho_wifi/test/test_native_api_validation`

## Related Issues

None tracked.

## AI Notes

Every new REST v2 endpoint must add a `processXxx` function (logic) plus a thin `handleGetXxx`/`handlePostXxx` (routing) — never combine them. See [.ai/common-patterns.md](../../.ai/common-patterns.md) #3 and [specs/templates/api-spec.md](../../specs/templates/api-spec.md).
