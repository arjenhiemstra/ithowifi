# ADR-0012: Servicetool bridge — interface-cable emulation with exclusive bus sessions

Status: Proposed

## Context

The official Itho servicetool talks to Itho devices through a dealer-only serial interface cable. The add-on already sits on the same I2C bus, as `0x80` — the address the servicetool uses as source. The cable's serial protocol is a thin wrapper: raw I2C frames in `DLE STX … DLE ETX` with DLE stuffing, plus a `DLE SYN` ping that must be echoed within 1 s. Exposing that over TCP lets the servicetool run against the add-on via any virtual COM port.

Constraints: the add-on's own I2C traffic (status polling, API/MQTT commands) would interleave with the servicetool's request/reply pairs; the ESP32 uses one I2C peripheral for master and slave (it only listens in a window after sending); the servicetool stops working when a ping is answered late.

## Decision

1. A dedicated task `TaskServicetool` owns a raw TCP listen socket and at most one client. It is created in `boot()` and gates on `PHASE_CONFIG | PHASE_NET` (ADR-0010).
2. Each frame from the servicetool is executed as one item on the existing I2C command queue, holding `I2CBusLock` across send + receive. The bridge task's client loop is non-blocking: it answers pings immediately, queues frames, forwards finished replies, and aborts the in-flight receive when the servicetool sends its next frame.
3. A connected client is an **exclusive session**: `servicetoolSessionActive` (polled flag, ADR-0004) pauses the add-on's periodic I2C work in `TaskSysControl` and makes the I2C driver refuse every send/read whose origin is not `I2C_CMD_SERVICETOOL`. The APIs refuse Itho control commands up front with a clear error (REST 409, MQTT `fail`).
4. Firmware/bootloader message classes are never forwarded over the bridge.
5. Session state is additive surface: retained MQTT topic `<base>/servicetool`, `GET /api/v2/servicetool`, HA discovery entities. Disabled by default.

## Consequences

**Positive**: the full servicetool works without the cable; no interleaving with add-on traffic; one catch-all guard in the driver covers code paths the API guards miss; protocol code is header-only and natively tested.

**Negative**: an unauthenticated TCP port when enabled (mitigated: off by default, optional allowed IP); during a session the add-on's own status data goes stale (optionally shown as unavailable in HA); the single-peripheral listening gap means late or unsolicited frames can be missed; frames are serialised, so throughput is bounded by the I2C queue.

## Alternatives Considered

- **Emulate the servicetool's USB interface**: needs USB-device hardware on the add-on side — more work, no benefit over a virtual COM port.
- **MQTT as transport** with a PC-side bridge: more moving parts, higher latency than raw TCP.
- **Blocking only at the API layer**: misses internal paths (virtual remotes, timers, sensor reads); the driver-level guard is the reliable choke point.
- **Always listening via the second I2C peripheral** (sniffer pins): closes the listening gap but needs hardware-specific work; possible follow-up.

## Related Components

`main/managers/ServicetoolBridge.{h,cpp}`, `main/managers/ServicetoolProtocol.h`, `main/ithodevice/i2c_esp32.cpp` (`servicetoolBlocks()`, `i2cSlaveReceive` overloads), `main/tasks/task_syscontrol.cpp`, `main/main.cpp`. Spec: [specs/active/servicetool-bridge.md](../../specs/active/servicetool-bridge.md).

## Related Issues

—

## AI Notes

- Never route servicetool frames around the I2C queue or call the driver directly from `TaskServicetool` — serialisation and `I2CBusLock` are what keep the bus consistent.
- New I2C call sites automatically inherit the session guard as long as they pass a real `i2c_cmdref_t` origin; don't add a new "bypass" origin.
- Don't relax the firmware/bootloader block list.
- New Itho control endpoints (REST/MQTT) should return the same "servicetool session active" error during a session.
