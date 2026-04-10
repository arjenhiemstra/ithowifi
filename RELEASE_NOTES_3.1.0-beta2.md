## Version 3.1.0-beta2

Maintenance release focused on the firmware update flow. Adds a dedicated REST resource for OTA so external tools (like the [Home Assistant integration](https://github.com/arjenhiemstra/ithowifi-ha-integration)) can show update status and trigger installs natively, and fixes the silent firmware-check failure that left `latest_fw` / `latest_beta_fw` empty on devices with a slightly larger `firmware.json`.

**⚠️ This is a beta release — please report issues at https://github.com/arjenhiemstra/ithowifi/issues**

### Highlights

- **New REST resource `/api/v2/ota`** — single endpoint for firmware version info AND OTA install/progress tracking
- **Compile-time sized `firmware.json` response cap** — no more silent failures when `firmware.json` grows past a hardcoded limit
- **Better error logging in `checkFirmwareUpdate()`** — every failure path now logs why
- **`firmware.json` build script** — generates GitHub release-download URLs instead of `raw/master/...` links

### New Features

- **`GET /api/v2/ota`** — returns `installed_version`, `latest_fw`, `latest_beta_fw`, `fw_update_available`, and live `state` / `progress` for an in-progress OTA install. State is one of `idle`, `starting`, `downloading`, `done`, `error`.
- **`POST /api/v2/ota`** — trigger a firmware install. Body is either `{"channel":"stable"}`, `{"channel":"beta"}`, or `{"url":"https://github.com/arjenhiemstra/ithowifi/..."}` for a pinned release asset. The URL form is domain-locked to the project's GitHub repo.
- **OpenAPI spec updated** with the new GET / POST and the `OtaInstallRequest` schema. Visible in the device's Swagger UI.

### Bug Fixes

- **`firmware.json` fetch silently failed** when the file grew past 2000 bytes, leaving `latest_fw` / `latest_beta_fw` empty and breaking the device's firmware update card. The cap is now derived from the actual `firmware.json` size at compile time with ample headroom, and overridable by the `MAX_FIRMWARE_HTTPS_RESPONSE_SIZE` build flag.
- **`checkFirmwareUpdate()` had no error logging** on any failure path. Every step now emits an `E_LOG` line on failure (alloc, `https.begin`, `GET`, HTTP status, oversized response, JSON parse, missing `hw_rev` entry), making remote diagnosis possible without serial access.
- **`build_script.py` wrote `raw/master/...` URLs** into `firmware.json` for new releases, which 404 if the binary isn't on master yet. The script now writes `https://github.com/arjenhiemstra/ithowifi/releases/download/Version-X.Y.Z/...` URLs that always resolve via the GitHub release.
- **`secclient` leaks** in `checkFirmwareUpdate()` — the `WiFiClientSecure` allocation was leaked on every error path. All paths now `delete secclient` before returning.

### API

- `GET /api/v2/ota` — new
- `POST /api/v2/ota` — new (with `channel` or `url` body)
- `POST /api/v2/debug` — unchanged (the existing `update` / `update_beta` / `update_url` actions there are deprecated and may be removed in a future release; new integrations should use `/api/v2/ota`)

### Compatibility

This release is fully backwards-compatible. The existing `POST /api/v2/debug` actions and the WebSocket-based `update_url` flow continue to work unchanged. The web UI's Update page is unaffected.

The Home Assistant integration version `0.4.0` of [ithowifi-ha-integration](https://github.com/arjenhiemstra/ithowifi-ha-integration) requires this firmware (3.1.0-beta2 or newer) to register the firmware update entity. Older firmware will simply omit the entity rather than fail.

### Firmware binary

Firmware binary (CVE HW rev.2 and NON-CVE):
https://github.com/arjenhiemstra/ithowifi/releases/download/Version-3.1.0-beta2/nrgitho-v3.1.0-beta2.bin
