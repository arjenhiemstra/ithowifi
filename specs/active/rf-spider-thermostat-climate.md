# Feature Spec: RF Spider thermostat (heating setpoint / climate)

Status: Active
Owner: Arjen Hiemstra
Related ADR(s): [ADR-0011](../../docs/adr/ADR-0011-rf-spider-thermostat-role.md) (touches [ADR-0005](../../docs/adr/ADR-0005-mqtt-integration-design.md) MQTT/HA surface and [ADR-0007](../../docs/adr/ADR-0007-config-persistence-littlefs-nvs.md) remote record)

## Summary

Let the add-on act as the RF heating controller of an **Itho Spider room thermostat**: read its room temperature and setpoint, push a setpoint to it, mirror manual dial changes, and expose it as a Home Assistant `climate` entity. Until now the firmware only knew the Spider as a *ventilation* remote (`RFTSPIDER`, fan speeds, join template `22F1/10E0/1FC9`).

Reference: the community HA blueprint [`Onl1ne1373/home-assistant-blueprints/spider.yaml`](https://github.com/Onl1ne1373/home-assistant-blueprints/blob/main/spider.yaml), which does this with HA automations on top of a RAMSES gateway (HGI80/evofw3). This feature removes the need for the gateway and the blueprint.

## Protocol (verified against real captures)

Captures from a CCU-12T20 Spider (`54:D3:FA`) on 2026-10-15, RF log level 3. **Roughly half of the received Spider frames were corrupted** (bit flips in ids, opcodes, lengths), so every handler requires a valid checksum.

### `0x01FF` Spider status (26 bytes)

Sent by the Spider as `RQ 01FF` to its heating controller, repeated when unanswered.

```
03 80 2B 28 2C D0 00 80 00 14 3C 28 40 00 00 01 04 80 80 02 80 FF 80 07 00 00
```

| Byte | Meaning | Encoding |
|---|---|---|
| 0 | `03` here, `00` in the blueprint and ramses_rf #101: device specific | |
| 2 | room temperature | 0.5 °C units, `0x80` = none (21.5 °C) |
| 3 | heating setpoint | 0.5 °C units (20.0 °C) |
| 4 | second (cooling) setpoint | 0.5 °C units (22.0 °C) |
| 5 | flags | bit 6 schedule active, bit 5 adjusted on the device (`D0`) |
| 13 | `00` / `10` between consecutive frames | unknown, varies |
| rest | device template | differs between Spiders (ramses_rf #101) |

Controller answer (`RP 01FF`) and push (`W 01FF`, 3x, 1 s apart) use the blueprint's fixed template, **not** an echo of the Spider's request:

```
00 80 80 <sp*2> <(sp+1)*2> D0 00 00 00 14 3C 80 80 00 00 B4 00 80 80 02 80 FF 80 04 00 00
```

### `0x22C9` zone setpoint

`[zone][setpoint BE16, 0.01 °C][second setpoint BE16][..]`. The Spider broadcasts `I 22C9 00 07D0 0898 00` (20.00 / 22.00) at power-up. A manual dial change arrives as `W 22C9` to the controller, which echoes `I 22C9 00 <sp*100 BE16> 08 02 01`. Note the **different encoding from 01FF** (0.01 °C in 2 bytes vs 0.5 °C in 1 byte).

### `0x1FC9` heating bind

The Spider's heating join (3x): `00 0008 <id> | 00 3150 <id> | 01 10E0 <id> | 00 1FC9 <id>` (relay demand, heat demand, device info, bind). The ventilation join offers `22F1` instead. The blueprint matched `0022`; this Spider sends `0008`, so the heating join is recognised by the **`3150`** entry. The controller accepts with `W 1FC9 00 01FF <controller id>`.

### Other frames seen at power-up

`30C9` room temperature (0.01 °C, already decoded), `12C0` (probably displayed temperature, 0.5 °C), `1060` battery, `3110`/`3120` (unknown), `10E0` device info (model `CCU-12T20`), `2E10` presence.

## Acceptance Criteria

- [ ] Firmware decodes `01FF` (checksum required) and exposes the Spider's room temperature, heating setpoint, second setpoint and schedule/manual flags in the remote's capabilities (web UI, REST `/api/v2/remotes`, MQTT `remotesinfo`), published as soon as a value changes.
- [ ] Heating bind: with the add-on in learn mode, a Spider heating join (`3150` entry) is accepted with `W 1FC9 00 01FF <our id>` and the slot is marked as thermostat.
- [ ] Firmware answers the bound Spider's `RQ 01FF` with `RP 01FF` and pushes `W 01FF` (3x) when the setpoint is changed via MQTT/REST/web UI; verified with the real Spider holding the new setpoint.
- [ ] Setpoint clamped to 5.0-30.0 °C on a 0.5 °C grid; pushes rate limited.
- [ ] Manual dial change (`W 22C9`) is adopted as the commanded setpoint and echoed with `I 22C9` (opt-in).
- [ ] HA `climate` entity (mode heat, current temp, target temp) via device-based discovery, opt-in per remote, drives the setpoint round trip.
- [ ] Existing `RFTSPIDER` ventilation behaviour unchanged.

## Architecture Impact

See [ADR-0011](../../docs/adr/ADR-0011-rf-spider-thermostat-role.md). Touches the RF parser/sender (`IthoCC1101`, `IthoPacket`), `task_cc1101` (bind branch, RQ responder, push scheduler), the remote record (`IthoRemote`: thermostat flag + commanded setpoint), MQTT/REST commands, HA discovery (`climate`), web UI.

## Risks

- **RF quality**: frequent corrupted frames. Mitigation: checksum gate on every Spider handler; native tests include a corrupted-length vector.
- **RQ to RP timing**: the Spider retries within about a second. Mitigation: the RP is sent from TaskCC1101 itself with a single try, not from another task.
- **No radio lock** in the firmware: other tasks send directly. Mitigation: all Spider transmissions are owned by TaskCC1101 (pending flags); the general lock is out of scope.
- **Template acceptance**: the controller template is the blueprint's; acceptance by this Spider is verified on hardware.
- **Binding moves the Spider away from its current controller** (`79:0C:17` in the capture). Documented in the UI/manual; re-pairing restores it.
- **Drives real heating**: clamp + rate limit; not safety critical.

## Test Requirements

- [x] Native `test_native_spider`: 01FF decode of the real captures and ramses_rf #101, rejects bad length/placeholder, controller template byte-exact vs the blueprint, round trip + clamp, 22C9 decode/echo, encodings do not cross, heating vs ventilation join detection, bind accept.
- [ ] Live hardware (.226 + Spider `54:D3:FA`): read values match the Spider display; bind; RQ answered; MQTT setpoint held by the Spider; manual dial mirrored; HA climate round trip; Spider fan commands still work.

## Rollback Strategy

All new behaviour is gated on a remote slot being marked thermostat (absent in existing configs, so dormant). Decoding 01FF is passive. Reverting the firmware leaves the extra remote keys ignored. The climate entity is opt-in and can be withdrawn via discovery without a firmware rollback.

## Related Components

`cc1101/spider_protocol.h` (pure payload helpers) · `cc1101/IthoPacket.h` (`SPIDERSTATUS`) · `cc1101/IthoCC1101.{h,cpp}` (`handleSpiderStatus`, senders, bind branch) · `tasks/task_cc1101.cpp` (capabilities, RQ responder, push scheduler) · `tasks/task_mqtt.cpp` (`updateMQTTRemotesInfo`) · `config/IthoRemote.{h,cpp}` · `config/HADiscConfig.cpp` + `HADiscovery.cpp` · `api/MqttAPI.cpp`, `api/WebAPIv2*.cpp` · `webroot_source/` · `test/test_native_spider/`.
