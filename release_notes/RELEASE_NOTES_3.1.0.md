## Version 3.1.0

> **⚠️ This version is superseded by [3.1.3](https://github.com/arjenhiemstra/ithowifi/releases/tag/Version-3.1.3).** Please upgrade to 3.1.3.

### Highlights

- **RF standalone mode** — full RF-only operation for devices without I2C (e.g. HRU 400), with guided first-boot setup wizard including interactive RF Join
- **OTA firmware update** — download and flash from GitHub releases via the web UI or REST API. New `GET/POST /api/v2/ota` enables firmware updates from external tools like the [Home Assistant integration](https://github.com/arjenhiemstra/ithowifi-ha-integration)
- **Channel-aware HA MQTT discovery** — auto-detects stable vs beta channel; beta users no longer see bogus "downgrade to stable" prompts ([#358](https://github.com/arjenhiemstra/ithowifi/issues/358))
- **Per-remote tracking** — new `last_cmd` and `presets` fields per remote in `/api/v2/remotes` and `/api/v2/vremotes`
- **Preset tables corrected** to match actual RF command maps (several remote types had missing presets)
- **Update page rewrite** — single firmware dropdown, inline install button, embedded progress bar
- **Swagger UI crash fixed** — OpenAPI spec moved to compile-time generation, eliminating stack overflow on the AsyncTCP task

### New REST API Endpoints

- `GET /api/v2/ota` — firmware versions, update channel, OTA progress
- `POST /api/v2/ota` — trigger install (`{"channel":"stable|beta"}` or `{"url":"..."}`)

### API Changes

- `/api/v2/remotes` — new `remotes` array with full config + `last_cmd` + `presets` per remote (backwards-compatible; `remotesinfo` unchanged)
- `/api/v2/vremotes` — now returns data (was broken), includes `last_cmd` + `presets` per virtual remote
- `/api/v2/deviceinfo` — new fields: `itho_control_interface`, `add-on_update_channel`, `add-on_latest_for_channel`
- MQTT command topic accepts `{"update":"auto"}` (resolves to stable/beta based on running version)

### Bug Fixes

- `firmware.json` fetch silently failed when file exceeded 2KB — now compile-time sized with headroom
- `checkFirmwareUpdate()` had no error logging — every failure path now logged
- `WiFiClientSecure` leaked on error paths in `checkFirmwareUpdate()`
- `firmware.json` URLs pointed at `raw/master/...` (404-prone) — now use release-download URLs
- `GET /api/v2/vremotes` returned empty `{}` — fixed (`as<>` vs `to<>` on fresh JsonDocument)
- OpenAPI spec handler caused device lockups (stack overflow on 4KB AsyncTCP task) — moved to compile-time gzip'd blob
- `CONFIG_ASYNC_TCP_STACK_SIZE` doubled from 4KB to 8KB as defense in depth

### Compatibility

Fully backwards-compatible with 3.0.0. All existing REST, MQTT, and WebSocket interfaces continue to work. New fields and endpoints are additive.

The [ithowifi-ha-integration](https://github.com/arjenhiemstra/ithowifi-ha-integration) v0.4.0+ requires this firmware for the native update entity; v0.5.0 requires it for per-remote fan entities.

### Firmware binary

https://github.com/arjenhiemstra/ithowifi/releases/download/Version-3.1.0/nrgitho-v3.1.0.bin
