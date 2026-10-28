# ADR-0011: RF thermostat role for the Itho Spider

Status: Accepted (revised 2026-10-27: controller spoofing replaces binding as the main mode)

## Context

The Itho Spider is both a ventilation remote and a heating room thermostat. The firmware handled only the ventilation side. Supporting the heating side means reading the Spider's room temperature, setpoint and heat demand, and setting its setpoint from MQTT/REST/web and Home Assistant. Spec: [rf-spider-thermostat-climate.md](../../specs/active/rf-spider-thermostat-climate.md).

The first design made the add-on the Spider's RF heating controller through the Spider's heating bind. Captures and on-air tests (2026-10-15 to 2026-10-18, two Spider types) changed that picture:

- A Spider is already bound to a heating controller: the boiler/heater (`3EF0` polls) or a relay such as a Honeywell BDR91 (`3EF1` polls). Binding it to the add-on takes it away from that controller, which users do not want.
- A Spider accepts `W 01FF` (set) and answers `RQ 01FF` (read) when the frame comes from the device id of its own controller. Ids of an old or other controller are ignored.
- A Spider is battery powered and listens only briefly: right after its controller answered its poll (`RP 3EF0` / `RP 3EF1`), and for some Spiders right after their own `I` broadcasts. Relay-bound Spiders poll only every 5 to 10 minutes when idle and about once a minute while heating. Touching the display wakes the Spider at once and it then accepts a pending setpoint within a second.
- Many Spider frames arrive corrupted; only checksum-valid frames can be trusted.
- The controller id is not visible in the Spider's own broadcasts, only in its polls and the controller's answers.

## Decision

1. **Thermostat is a capability of a remote slot**, not a separate subsystem. A Spider is an `RFTSPIDER` slot (function Monitor when found by discovery, so its fan commands are not executed) with optional persisted keys `thermostat`, `setpoint_cmd` (commanded setpoint) and `ctlid` (its controller). Absent keys mean the feature is off, so existing configs are untouched.
2. **Main mode: act as the Spider's own controller.** The add-on sends `W 01FF` and `RQ 01FF` with the learned controller id as source. The Spider stays bound to its heater or relay, which keeps working normally. The bind remains available through learn mode for the Spider's controller join (the "home icon" pairing, which offers `22C9`): the add-on accepts with `W 1FC9 00 01FF <own id>`, becomes the controller (`ctlid` empty) and answers the Spider's `RQ 01FF`. A heater/relay join (offers `3150`/`0008`) is not answered; the user is pointed to discovery.
3. **Discovery wizard to find the Spider and its controller.** The user sets the Spider to one random setpoint between 21.5 and 24 °C (Spiders on an AutoTemp go up to 24 °C). Spider/controller pairs are learned from polls and answers at all times. A Spider that raises its heat demand (`I 0008`) becomes a candidate (max 4) and is asked up to 3 times with `RQ 01FF` as its controller, in its poll window; the setpoint in the answer, or a `22C9` broadcast, identifies it. A wrong value rejects the candidate. A Spider on a zone controller (AutoTemp, seen in a user log 2026-10-28) does not poll with `01FF`/`3EF0`/`3EF1`: it sends a dial change as `W 22C9 <zone>,<setpoint>,<setpoint+1 °C>,<flag>` to the controller, which echoes it as `I 22C9` from its own ID, and it sends `RQ 0008 <zone>` and `I 30C9` to the controller. The `W 22C9` identifies the Spider and its zone controller; an `I 22C9` from a known controller is ignored. Such a Spider is stored read only (no thermostat role), since we cannot pose as its controller; setting it via the zone controller is open. The logic is a dependency-free class in `cc1101/spider_discovery.h`.
4. **Send only in the Spider's listen window, from TaskCC1101.** MQTT/REST/web only store the setpoint and request a push. The RF task sends `W 01FF` (plus 2 copies 1 s apart) right after the controller's `RP` to the Spider, or 300 ms after the Spider's poll if that answer was missed, and after plain `I` broadcasts at most once per 30 s (never after `I 0008`, which the heater echoes). A push stays pending until the Spider reports the commanded setpoint, or 15 minutes pass. A status `RQ 01FF` is sent every 5 minutes. Sends use the shared radio lock (`radioSem`).
5. **Payload logic lives in a dependency-free header** (`cc1101/spider_protocol.h`) with native tests. Controller frames use the fixed blueprint template; the setpoint is clamped to 5 to 30 °C in 0.5 °C steps.
6. **Every Spider handler requires a valid checksum**, independent of the global `CRC_FILTER` setting.
7. **Read values are published on change** via the `updateMQTTRemotesInfo` flag, and as capabilities on the remote (`temp`, `setpoint`, `setpoint_cool`, `setpoint_cmd`, `demand`, `schedule`, `manual`, `thermostat`).
8. **API surface is additive**: MQTT `{"rfsetpoint": <°C>, "rfremoteindex": <n>}`, REST `POST /api/v2/rfremote/setpoint`, a websocket message for the web UI. Home Assistant gets a `climate` entity per thermostat slot from the HA custom integration (opt-in per Spider in its setup), not from MQTT discovery. No existing topic or path changes.

## Consequences

- Positive: no RAMSES gateway or HA blueprint needed, and the Spider keeps controlling the heating through its own heater or relay. Pure protocol and discovery logic is unit tested.
- Negative: the add-on transmits frames with another device's id; a wrong learned controller id could confuse a heater, so all sending is gated on a slot marked thermostat with a learned `ctlid`. The discovery wizard briefly raises the user's heating setpoint and probes Spiders within range that raised their demand, which may include a neighbour's Spider (max 3 read requests each, no writes).
- Negative: a setpoint is applied only when the Spider listens, which can take up to 10 minutes for an idle relay-bound Spider; a push can expire unconfirmed. Touching the Spider's display makes it apply at once (to be stated in the UI and manual).
- The fixed controller template is verified on two Spider types only.

## Alternatives Considered

- **Bind the Spider's heating side to the add-on** (the first design): kept only as a secondary mode, because it removes the Spider from its heater or relay.
- **Use an old Itho gateway id as source**: did not work after re-pairing; the Spider only accepts its current controller.
- **Discovery with two setpoint steps** (down, then up): replaced by one random high setpoint, which is unique and raises heat demand sooner, so relay-bound Spiders start polling.
- **Send at any time with many retries**: frames outside the listen window are ignored and only add air traffic.
- **Separate single-thermostat setting** outside the remotes list: limited to one Spider and duplicates id/persistence/UI handling.

## Related Components

`cc1101/spider_protocol.h`, `cc1101/spider_discovery.h`, `cc1101/IthoCC1101.{h,cpp}`, `cc1101/IthoPacket.h`, `tasks/task_cc1101.cpp`, `tasks/task_mqtt.cpp`, `config/IthoRemote.{h,cpp}`, `generic_functions.cpp` (`ithoSetSpiderSetpoint`), `api/MqttAPI.cpp`, `api/WebAPIv2Rest.cpp`, `websocket.cpp`, web UI RF devices page, `test/test_native_spider/`, `test/test_native_spider_discovery/`, HA integration `climate.py`.

## Related Issues

None yet.

## AI Notes

- Do not send Spider frames from MQTT/web context; set state and let TaskCC1101 transmit in the listen window.
- Never send as a controller id that was not learned for that Spider (`ctlid`).
- Keep 01FF (0.5 °C, 1 byte) and 22C9 (0.01 °C, BE16) conversions in `spider_protocol.h` only.
- Join kinds: `22C9` offered = controller join (bind to us), `3150` offered = heater/relay join (never answer; use discovery). See `spider::heatingJoinKind`.
- Never act on a Spider frame with a checksum error.
- Do not send right after the Spider's `I 0008`: the heater answers it with `3150` and the frames collide.
