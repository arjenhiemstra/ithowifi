# ADR-0011: RF thermostat role for the Itho Spider

Status: Accepted

## Context

The Itho Spider is both a ventilation remote and a heating room thermostat. The firmware handled only the ventilation side. Supporting the heating side means the add-on becomes the Spider's RF **heating controller**: it must accept the Spider's heating bind, answer its `RQ 01FF` status requests, push setpoints with `W 01FF`, and expose the result over MQTT/REST/HA. This adds a new role (we answer requests, which the firmware never did), new persisted state, a new HA component type and new commands. Spec: [rf-spider-thermostat-climate.md](../../specs/active/rf-spider-thermostat-climate.md).

Constraints found in the code: RF transmissions are made directly from the MQTT, web and syscontrol tasks without a radio lock; received packets have a single pending slot; checksum failures are not filtered globally (`CRC_FILTER` off); `remotesinfo` is only published on the periodic status tick.

## Decision

1. **Thermostat is a capability of a remote slot**, not a separate subsystem. A Spider stays an `RFTSPIDER` slot in the remotes list; the slot gets a thermostat flag and the commanded setpoint, persisted as optional keys in the remote record (absent = feature off, so existing configs are untouched).
2. **Payload logic lives in a dependency-free header** (`cc1101/spider_protocol.h`) so it is covered by native tests. Controller frames (`RP`/`W 01FF`) use the blueprint's fixed template; the Spider's own request bytes are decoded, not echoed.
3. **Every Spider handler requires a valid checksum**, independent of the global `CRC_FILTER` setting.
4. **All Spider transmissions are owned by TaskCC1101.** MQTT/REST/web only set the desired setpoint; the RF task sends the `RP` (one try, immediately when the `RQ` arrives) and the `W` pushes (3x, 1 s apart). This keeps reply timing predictable without introducing a general radio lock.
5. **Read values are published on change** via a dedicated `updateMQTTRemotesInfo` flag instead of waiting for the status tick.
6. **API surface is additive**: MQTT command `{"rfsetpoint": <°C>, "rfremoteindex": <n>}`, REST `POST /api/v2/rfremote/setpoint`, new capability keys (`setpoint_cool`, `schedule`, `manual`), and an opt-in HA `climate` component per thermostat slot. No existing topic or path changes.

## Consequences

- Positive: no external RAMSES gateway or HA blueprint needed; reuses remotes persistence, UI and discovery plumbing; pure protocol code is unit tested.
- Negative: binding the Spider's heating side to the add-on takes it away from its previous controller. The fixed controller template may not suit every Spider variant (verified per device on hardware). The wider lack of a radio lock remains; this ADR only keeps Spider traffic out of it.

## Alternatives Considered

- **Separate single-thermostat setting** outside the remotes list: simpler, but limited to one Spider and duplicates id/persistence/UI handling.
- **Echo the Spider's own 01FF bytes as the controller answer** (the original spec idea): rejected after captures showed the controller template differs from the Spider's request in several bytes.
- **Send from the calling task** like the existing RF commands: rejected because the `RQ`/`RP` window is short and other tasks can be mid-transmission.
- **General radio mutex/queue for all RF sends**: right long-term, but a larger refactor with regression risk across all remote types; out of scope.

## Related Components

`cc1101/spider_protocol.h`, `cc1101/IthoCC1101.{h,cpp}`, `cc1101/IthoPacket.h`, `tasks/task_cc1101.cpp`, `tasks/task_mqtt.cpp`, `config/IthoRemote.{h,cpp}`, `config/HADiscConfig.cpp`, `HADiscovery.cpp`, `api/MqttAPI.cpp`, `api/WebAPIv2.cpp`, `api/WebAPIv2Rest.cpp`, `test/test_native_spider/`.

## Related Issues

None yet.

## AI Notes

- Do not send Spider frames from MQTT/web context; set state and let TaskCC1101 transmit.
- Keep 01FF (0.5 °C, 1 byte) and 22C9 (0.01 °C, BE16) conversions in `spider_protocol.h` only.
- Recognise the heating join by the `3150` entry, not by a fixed first code (Spiders differ: `0008` vs `0022`).
- Never act on a Spider frame with a checksum error.
