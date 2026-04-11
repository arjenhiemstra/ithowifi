## Version 3.1.0-beta4

Corrects the per-remote-type preset tables used by HA Auto Discovery and exposes them via the REST API so external integrations (HA custom integration, scripts) know exactly which commands each configured remote supports.

**⚠️ This is a beta release — please report issues at https://github.com/arjenhiemstra/ithowifi/issues**

### Highlights

- **Preset tables now match the actual RF command maps** in `IthoCC1101.cpp`. Previously several remote types had a too-narrow preset list (e.g. RFT RV / RFT CO2 / RFT CVE). The published presets are now derived from the same `RFT*_Remote_Map[]` that the radio uses for transmission.
- **New `presets` field on every remote entry** in `GET /api/v2/remotes` and `GET /api/v2/vremotes`, in wire format (e.g. `low,medium,high,timer1,timer2,timer3`). Intended for external consumers that need to know which commands each remote supports without duplicating the table client-side.
- **Web UI label fix**: the `itho_vremoteapi` setting's label was misleading — it's now labelled "Map API key "command" to virtual remote 0", which is what it actually does.

### Bug Fixes

- **RFT CVE remotes were missing `Away`** from the preset list. The RF command map has a valid `ithoMessageAwayCommandBytes` entry for CVE. Added.
- **RFT RV remotes were missing `Low` and `High`**. Same cause. Added.
- **RFT CO2 remotes were missing `Low` and `High`**. Same cause. Added.
- **RFT AutoN remotes were missing `Away` and `Medium`** (they were sharing a preset row with RFT Auto, which is a different type). Split into their own case with the correct set: `Away,Low,Medium,High,Auto,AutoNight,Timer 10/20/30min`.
- **RFT Spider remotes were missing almost everything** — the preset list was just `Low,High` but the RF command map supports Low, Medium, High, Auto, AutoNight, and all three timers. Full list now published.

### API

- **CHANGED**: `GET /api/v2/remotes` — `remotes[*]` entries now include a `presets` field (wire-format comma-separated string) reflecting the commands supported by each remote's type.
- **CHANGED**: `GET /api/v2/vremotes` — same addition on `vremotesinfo[*]`.
- **UNCHANGED**: `remotesinfo` shape, MQTT publish format, existing preset commands — all backwards-compatible.

### Internal

- New `HADiscConfig::getWirePresetsForType(uint16_t remoteType)` static helper, declared in `HADiscConfig.h` and defined in `HADiscConfig.cpp`. Kept in lockstep with the existing human-readable `getPresetsForType()` used by HA MQTT auto-discovery.
- `IthoRemote::Remote::get()` now emits `obj["presets"]` for every remote it serializes.

### Compatibility

Fully backwards-compatible with 3.1.0-beta3.

- Existing consumers of `/api/v2/remotes` and `/api/v2/vremotes` see only new fields; nothing removed or renamed.
- HA MQTT Auto Discovery users see the corrected preset list on their remotes' `select` entities after the next discovery cycle. Automations that previously worked with the (too-narrow) list still work; automations that tried to use a preset the device silently ignored may now actually apply.
- The Home Assistant integration [ithowifi-ha-integration](https://github.com/arjenhiemstra/ithowifi-ha-integration) version `0.5.0` requires this firmware release to get accurate per-remote preset lists. On older firmware (`3.1.0-beta3` or earlier) it falls back to a conservative `low / medium / high` set.

### Firmware binary

Firmware binary (CVE HW rev.2 and NON-CVE):
https://github.com/arjenhiemstra/ithowifi/releases/download/Version-3.1.0-beta4/nrgitho-v3.1.0-beta4.bin
