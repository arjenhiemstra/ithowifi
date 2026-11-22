# Feature Spec: Servicetool bridge (Itho interface cable over TCP)

Status: Active
Owner: WouterJN
Related ADR(s): [ADR-0012](../../docs/adr/ADR-0012-servicetool-bridge.md)

## Summary

Lets the official Itho Daalderop servicetool (Windows) talk to the Itho through the add-on, without the dealer-only Itho interface cable. The add-on emulates the serial framing of that cable on a raw TCP socket; on Windows a virtual COM port (e.g. HW VSP3 in raw TCP client mode) maps a COM port to `<add-on-ip>:<port>`. While a servicetool is connected (a *session*) the bus belongs to the servicetool: the add-on pauses its own I2C polling and refuses Itho control commands from all other interfaces. Off by default.

Wire format (as used on the interface cable's serial line): raw I2C frame wrapped in `DLE STX … DLE ETX` with every `0x10` doubled; ping `DLE SYN` (`10 16`) must be answered within about 1 s, otherwise the servicetool does not recognise the interface or stops working. Implemented header-only in `managers/ServicetoolProtocol.h`.

## Acceptance Criteria

- [x] With the bridge enabled, the servicetool finds the interface (ping answered) and fully starts up against a WPU (identification broadcast to `0x00`, `0x2400`/`0x2401`/`0x2410`, hardware configuration, fault settings `0x4030`)
- [x] Pings are answered immediately, also while an I2C request is in flight
- [x] A new frame from the servicetool aborts the wait for the previous reply (no head-of-line blocking)
- [x] Firmware/bootloader message classes (`0x4035–0x4038`, `0x4F35–0x4F38`) are never forwarded
- [x] During a session: periodic I2C polling paused; Itho control commands refused on REST v2 (409), legacy API v1 (409), MQTT (`fail` on `<base>/cmd/response`) and at driver level (catch-all); module functions (status, OTA, debug/reboot, RF remote config) stay available
- [x] Session ends on disconnect, after `servicetool_timeout` minutes without traffic, when the bridge is disabled, or via the web UI; polling resumes with an immediate status refresh
- [x] Session state published retained on `<base>/servicetool`, exposed via `GET /api/v2/servicetool`, HA discovery `binary_sensor` + `sensor`, and a banner in the web UI
- [x] Web UI warns that NVT/Telnet (RFC2217) must be off in the virtual serial driver and that the bridge must never be used for firmware updates

## Architecture Impact

- Touches: task chain (new `TaskServicetool`, created in `boot()`, gates on `PHASE_CONFIG | PHASE_NET`), REST API (new `GET /api/v2/servicetool`, 409 on control endpoints during a session), MQTT topics (new retained `<base>/servicetool`), config format (5 new `SystemConfig` keys, all optional with defaults), I2C driver (`i2cSlaveReceive` overloads, session guard).
- New ADR: [ADR-0012](../../docs/adr/ADR-0012-servicetool-bridge.md). Follows ADR-0004 (flag polling), ADR-0005 (additive topic), ADR-0006, ADR-0007 (additive keys), ADR-0010 (phase gates).

## Risks

- **Raw TCP port has no authentication** — off by default, optional single allowed client IP, UI says so explicitly. No transport security is claimed.
- **Servicetool can change parameters / manual operation** — dealer tool; same risk as with the cable. Firmware flashing is blocked.
- **Listening gap** — the ESP32 uses one I2C peripheral for master and slave, so it only listens in a window after sending; a reply that arrives after the servicetool's next frame, or an unsolicited frame, is missed. Not observed in practice (WPU replies within tens of ms). Possible follow-up: idle listen windows, or a second I2C peripheral on the sniffer pins.
- **NVT in the virtual serial driver** mangles `0xFF` bytes → "Data niet goed opgehaald". Mitigated by the UI warning.
- Flash/RAM: ~+2.5 kB flash; one extra task (6 kB stack) only does work when enabled; per-frame request objects on the heap (~800 B) while in flight.

## Test Requirements

- [x] Native unit test: `test_native_servicetool` — checksum, frame validation, DLE encode/decode (incl. reference frame from the servicetool), overflow, splitting multiple replies collected in one receive window, blocked classes, reply windows
- [x] Manual verification on hardware: WPU 5G (list version 41) with ID_Servicetool 1.2.66.0 (build 178) via HW VSP3 3.1.2, NVT off
- [ ] Live-hardware automated coverage: none (needs the Windows servicetool)

## Rollback Strategy

Safe to `git revert`. Disabled by default; the new config keys are optional and ignored by older firmware. No persisted-state migration.

## Related Components

`main/managers/ServicetoolBridge.{h,cpp}`, `main/managers/ServicetoolProtocol.h`, `main/ithodevice/i2c_esp32.{h,cpp}`, `main/ithodevice/i2c_logger.h`, `main/managers/I2CManager.{h,cpp}`, `main/tasks/task_syscontrol.cpp`, `main/tasks/task_mqtt.cpp`, `main/main.cpp`, `main/api/WebAPIv2Rest.cpp`, `main/api/WebAPIv1.cpp`, `main/api/MqttAPI.cpp`, `main/HADiscovery.cpp`, `main/config/SystemConfig.{h,cpp}`, `main/handlers/WebSocketHandlers.cpp`, `main/websocket.cpp`, `main/webroot_source/…`, `build_script.py`, `docs/openapi.json`, `test/test_native_servicetool/`
