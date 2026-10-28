# Feature Spec: RF Spider thermostat (heating setpoint / climate)

Status: Active
Owner: Arjen Hiemstra
Related ADR(s): [ADR-0011](../../docs/adr/ADR-0011-rf-spider-thermostat-role.md) (touches [ADR-0005](../../docs/adr/ADR-0005-mqtt-integration-design.md) MQTT surface and [ADR-0007](../../docs/adr/ADR-0007-config-persistence-littlefs-nvs.md) remote record)

## Summary

Read and set an **Itho Spider room thermostat** over RF: room temperature, setpoint and heat demand in the web UI, REST and MQTT, a setpoint command, and a Home Assistant `climate` entity through the HA integration. The Spider stays bound to its own heater or relay; the add-on sends as that controller in the Spider's short listen windows (ADR-0011). Until now the firmware only knew the Spider as a *ventilation* remote (`RFTSPIDER`, fan speeds, join template `22F1/10E0/1FC9`).

Reference: the community HA blueprint [`Onl1ne1373/home-assistant-blueprints/spider.yaml`](https://github.com/Onl1ne1373/home-assistant-blueprints/blob/main/spider.yaml), which uses HA automations on top of a RAMSES gateway (HGI80/evofw3). This feature removes the need for the gateway and the blueprint.

## Protocol (verified on air)

Two Spiders, captured with an evofw3 RAMSES logger that can also transmit raw frames (2026-10-15 to 2026-10-18):

| Spider | Controller | Polls | Listens |
|---|---|---|---|
| `54:D3:FA` (CCU-12T20) | heater `29:39:D7` | `RQ 3EF0` | after the heater's `RP` and after its own `I` broadcasts (every ~3 min idle) |
| `54:BB:2F` | Honeywell BDR91 relay `36:28:8D` | `RQ 3EF1` every 5 to 10 min idle, ~1 min while heating | only around the relay poll; sends `22C9` only every ~14:36 |

Both: touching the display wakes the Spider at once. **Many received Spider frames are corrupted** (bit flips in ids, opcodes, lengths), so every handler requires a valid checksum.

### Reading and setting: act as the controller

- `RQ 01FF` sent with the controller's id as source is answered with `RP 01FF`.
- `W 01FF` sent with the controller's id sets the Spider; it answers `I 01FF` and broadcasts `I 22C9`. An id of an old controller (gateway `79:0C:17`) is ignored.
- Send right after the controller's `RP` to the Spider, or ~300 ms after the Spider's poll. Not after `I 0008`: the heater echoes it with `3150` and the frames collide. Idle `W` frames outside a window are ignored (verified: no answer for 10 minutes, applied within a second after the display was touched).

### `0x01FF` Spider status (26 bytes)

```
03 80 2B 28 2C D0 00 80 00 14 3C 28 40 00 00 01 04 80 80 02 80 FF 80 07 00 00
```

| Byte | Meaning | Encoding |
|---|---|---|
| 0 | `03` here, `00` in the blueprint and ramses_rf #101: device specific | |
| 2 | room temperature | 0.5 °C units, `0x80` = none |
| 3 | heating setpoint | 0.5 °C units |
| 4 | second (cooling) setpoint | 0.5 °C units |
| 5 | flags | bit 6 schedule active, bit 5 adjusted on the device |
| rest | device template | differs between Spiders |

`RP`/`W 01FF` from the controller use the blueprint's fixed template, not an echo of the request:

```
00 80 80 <sp*2> <(sp+1)*2> D0 00 00 00 14 3C 80 80 00 00 B4 00 80 80 02 80 FF 80 04 00 00
```

### Other frames

- `22C9` zone setpoint: `[zone][setpoint BE16, 0.01 °C][second setpoint BE16]..`. Different encoding from 01FF.
- `30C9` room temperature (0.01 °C), `0008` heat demand (0 to 200), `12C0` displayed temperature, `1060` battery, `10E0` device info.

### Joins

- Heater/relay join (offers `0008`/`3150`/`10E0`/`1FC9`): the heater accepts with `W 1FC9 00 3EF0 <heater>`. The add-on does not answer it.
- Controller join ("home icon", offers `22C9`/`22F1`/`000E`; seen in ramses_cc #151, not captured on our Spiders): accepted with `W 1FC9 00 01FF <controller>`. The add-on accepts it in learn mode and becomes the Spider's controller.

## Discovery

The Spider wizard on the RF remotes page finds a Spider and its controller:

1. The add-on shows one random setpoint between 21.5 and 24 °C; the user sets it on the Spider.
2. Spider/controller pairs are learned from polls and answers at all times.
3. A Spider broadcasting `22C9` with the target is found directly. A Spider that raises its heat demand (`I 0008`) becomes a candidate (max 4) and is asked up to 3 times with `RQ 01FF` as its controller, in its poll window; the `RP 01FF` setpoint identifies it, a wrong value rejects it.
4. Found: the Spider is stored on a free slot (function Monitor), marked thermostat, with `ctlid`. Timeout 20 min.

## Acceptance Criteria

- [x] Firmware decodes `01FF`, `22C9`, `30C9`, `0008` (checksum required) and exposes temperature, setpoint, second setpoint, commanded setpoint, demand and schedule/manual flags as remote capabilities (web UI, REST `/api/v2/remotes`, MQTT `remotesinfo`), published on change.
- [x] Discovery wizard finds a heater-bound and a relay-bound Spider with their controller.
- [x] Setpoint via MQTT `{"rfsetpoint":..,"rfremoteindex":..}`, REST `POST /api/v2/rfremote/setpoint` and the web UI; clamped to 5.0 to 30.0 °C on a 0.5 °C grid.
- [x] `W 01FF` sent as the controller in listen windows, 2 repeats 1 s apart, pending until the Spider reports the commanded setpoint or 15 min pass; status `RQ 01FF` every 5 min.
- [x] Manual change on the Spider shows up as the new setpoint (next `01FF`/`22C9`).
- [x] HA `climate` entity (HA integration v0.7.0): current temperature, target temperature, heating action, opt-in per Spider; shows the requested value until confirmed.
- [ ] Controller join accepted in learn mode and the add-on's `RP 01FF` answers held by a Spider (bind mode).
- [x] Existing `RFTSPIDER` ventilation behaviour unchanged; nothing is sent for slots without the thermostat flag.

## Architecture Impact

See [ADR-0011](../../docs/adr/ADR-0011-rf-spider-thermostat-role.md). Touches the RF parser/sender (`IthoCC1101`, `IthoPacket`), `task_cc1101` (listen windows, push/poll scheduler, discovery, bind branch), the remote record (`IthoRemote`: `thermostat`, `setpoint_cmd`, `ctlid`), MQTT/REST/websocket commands, web UI, and the HA integration (`climate.py`, config flow, Repairs).

## Risks

- **Sending as another device:** a wrong `ctlid` could confuse a heater. Mitigation: only sent for slots marked thermostat with a learned controller; payloads are fixed templates with a clamped setpoint.
- **Neighbours:** discovery may probe a neighbour's Spider that raised its demand (read only, max 3 requests).
- **Delay:** an idle relay-bound Spider can take up to 10 minutes to apply a setpoint, and a push can expire unconfirmed. Mitigation: 15 min deadline; touching the display applies it at once (UI/manual hint).
- **RF quality:** frequent corrupted frames. Mitigation: checksum gate on every Spider handler.
- **Template acceptance:** verified on two Spider types only.
- **Drives real heating:** clamp + bounded sending; not safety critical.

## Test Requirements

- [x] Native `test_native_spider`: 01FF decode of real captures and ramses_rf #101, bad length/placeholder rejected, controller template byte-exact vs the blueprint, round trip + clamp, 22C9 decode, encodings do not cross, join kinds, bind accept.
- [x] Native `test_native_spider_discovery`: target range, `22C9` match, heater learned after the Spider, relay Spider found by probe, wrong answer rejects, probe limit and no-demand, unrelated traffic ignored, idle/restart.
- [x] Live (.226 / .83 with `54:D3:FA` and `54:BB:2F`): values match the display, discovery, setpoint held, display-touch behaviour, HA climate round trip.
- [ ] Live: bind mode with a Spider's controller join.
- [ ] Optional: shorter fallback after a missed poll answer (100 to 300 ms) on a relay-bound Spider.

## Rollback Strategy

All new behaviour is gated on a remote slot marked thermostat (absent in existing configs, so dormant). Decoding is passive. Reverting the firmware leaves the extra remote keys ignored; the HA climate entity becomes unavailable.

## Related Components

`cc1101/spider_protocol.h`, `cc1101/spider_discovery.h`, `cc1101/IthoPacket.h`, `cc1101/IthoCC1101.{h,cpp}`, `tasks/task_cc1101.cpp`, `tasks/task_mqtt.cpp`, `config/IthoRemote.{h,cpp}`, `generic_functions.cpp`, `api/MqttAPI.cpp`, `api/WebAPIv2Rest.cpp`, `websocket.cpp`, `webroot_source/`, `test/test_native_spider/`, `test/test_native_spider_discovery/`, HA integration `climate.py`, `config_flow.py`, `repairs.py`.
