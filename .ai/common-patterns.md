# Common Patterns

Reusable patterns observed in the codebase. Follow these instead of inventing new structures.

## 1. Global manager singleton

Managers under `main/managers/` are declared as `extern <Class> <instance>;` and defined once, not constructed per-use.

```cpp
// main/managers/RFManager.h
extern RFManager rfManager;
// usage anywhere:
rfManager.radio.receivePacket();
```

Apply this when adding new stateful hardware/subsystem access — extend an existing manager or add a new one following the same extern-instance shape, don't wrap state in a locally-constructed object passed around.

## 2. Data-table device model

`main/ithodevice/IthoDevice.h` defines `struct ithoDeviceType` once; `main/ithodevice/IthoDevice.cpp:35` populates `ithoDevices[]` by including per-device headers from `main/ithodevice/devices/` (`cve14.h`, `hru350.h`, `wpu.h`, ...). Device-specific behavior = data (label/mapping tables), not virtual methods.

**To add a device**: add a `devices/<name>.h` with its label/mapping tables, add a row to `ithoDevices[]`. Do not create a subclass or new polymorphic type.

## 3. Logic/routing split (REST API)

```
main/api/WebAPIv2.cpp      processCommand(JsonDocument&) -> api_response_status_t   // pure logic
main/api/WebAPIv2Rest.cpp  handlePostCommand(request)  -> parses request, calls processCommand, writes response  // framework binding
```

This split is why `test_native_api_validation` can unit-test validation logic on a host machine without an ESP32 or network stack. Keep new endpoints in this shape.

## 4. Flag-polling task loop

Tasks don't wait on RTOS queues for most signaling — a task loop polls plain global flags each iteration and clears them after acting:

```cpp
if (saveSystemConfigflag) {
    saveSystemConfig(...);
    saveSystemConfigflag = false;
}
```

When adding new cross-task triggers, follow this flag convention rather than introducing `xQueueCreate` for task-to-task messaging (the task chain uses flags; the only queue today is the I2C sniffer's ISR event queue — see [docs/adr/ADR-0004-freertos-linear-task-chain.md](../docs/adr/ADR-0004-freertos-linear-task-chain.md) before adding one).

## 5. I2C command-queue closures

`I2CManager` (`main/managers/I2CManager.h`) holds a mutex-guarded `std::deque<std::function<void()>>`. `main/handlers/I2CQueryHandlers.cpp` registers closures (`initI2cFunctions()`) that get enqueued and drained by the I2C task loop — this decouples "what to query/set" from "when the bus is free."

## 6. JSend API responses

`ApiResponse` (`main/api/ApiResponse.h`) builds the standard `{status, data|message}` envelope. Reuse it for any new REST or MQTT command response instead of hand-building JSON.

## Related

[.ai/coding-standards.md](coding-standards.md) · [.ai/architecture-summary.md](architecture-summary.md)
