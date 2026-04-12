## Version 3.1.0-beta2

Adds an RF standalone setup wizard for devices with no I2C connection (e.g. HRU 400), a redesigned firmware update page, a dedicated REST resource for OTA so external tools (like the [Home Assistant integration](https://github.com/arjenhiemstra/ithowifi-ha-integration)) can natively show update status and trigger installs, and several smaller API additions plus bug fixes.

**⚠️ This is a beta release — please report issues at https://github.com/arjenhiemstra/ithowifi/issues**

### Highlights

- **RF standalone setup wizard** — first-boot wizard now detects "no I2C device" and offers a guided RF-only setup path including automatic RF remote config and an interactive Join step
- **Redesigned Update page** — single firmware dropdown listing all available versions (stable, beta, historical), inline install button, embedded progress bar driven by live OTA status
- **New REST resource `/api/v2/ota`** — single endpoint for firmware version info AND install/progress tracking, intended for HA and other integrations
- **`/api/v2/remotes`** now exposes full remote config alongside the existing `remotesinfo` (backwards compatible)
- **`itho_control_interface`** added to `/api/v2/deviceinfo`
- **`firmware.json` fixes** — release-download URLs (no more 404s when binary isn't on master), compile-time sized response cap (no more silent failures when the file grows past 2KB), full error logging on every failure path

### New Features

#### RF standalone setup wizard

- New "**RF standalone mode**" option in System Settings (CC1101 section) for devices with no I2C connection
- Wizard auto-detects the no-I2C scenario (Generic Itho device / no detected I2C type) and presents an info box: *"No I2C connection detected. If your Itho device does not have an I2C connector (e.g. HRU 400), you can use RF standalone mode."* Two buttons: **Use RF Standalone Mode** or **Keep trying I2C**
- In RF standalone mode the wizard:
  - Hides the I2C section and disables I2C status queries (`itho_pwm2i2c=0`, `itho_31da=0`, `itho_31d9=0`, `itho_4210=0`, `itho_sendjoin=0`, `itho_forcemedium=0`, `itho_numvrem=0`)
  - Auto-configures RF remote slot 0 as **Send + RFT CO2** (`0x1298`)
  - Adds a **Join** step (Step 3) to pair the add-on with the Itho unit interactively, with success/failure feedback via a new `bind_result` websocket message
  - Sets `itho_rf_standalone=1` and `itho_control_interface=1` on finish
  - Skips the post-install power-cycle step (just reboot is enough since join already happened in the wizard)
- When `itho_rf_standalone=1` is active, the I2C, virtual remotes, and status menus are hidden in the web UI
- Wizard also recognises "Generic Itho device" as a no-device-type scenario

#### Update page rewrite

- Single dropdown listing **all available firmware versions** populated from `firmware.json` — current stable, current beta, and any historical versions in the `versions` array. Current version is annotated with "- current"
- **Show beta firmware version(s)** checkbox to filter the list
- Single **Install** button (replaces the separate "download stable / download beta" buttons) plus a single **Release notes** link
- **Embedded progress bar** that shows download/flash percentage live, driven by the `systemstat.ota_progress` websocket field
- **2-minute stuck-progress watchdog** — if no progress update arrives for 2 minutes the install is considered stuck and the UI shows an error
- Auto-reload after device returns from reboot
- Manual `.bin` upload still works as before (fieldset renamed to "Manual firmware update")

#### REST API additions

- **`GET /api/v2/ota`** — returns `installed_version`, `latest_fw`, `latest_beta_fw`, `fw_update_available`, plus live `state` and `progress` for an in-progress install. State is one of `idle`, `starting`, `downloading`, `done`, `error`
- **`POST /api/v2/ota`** — trigger a firmware install. Body is either `{"channel":"stable"}`, `{"channel":"beta"}`, or `{"url":"https://github.com/arjenhiemstra/ithowifi/..."}` for a pinned release asset. The URL form is domain-locked to the project's GitHub repo
- **OpenAPI spec updated** with the new GET / POST endpoints and the `OtaInstallRequest` schema. Visible in the device's Swagger UI
- **`GET /api/v2/remotes`** now returns the full remote configuration in a new top-level `remotes` array. The existing `remotesinfo` object is preserved for backwards compatibility
- **`GET /api/v2/deviceinfo`** now includes `itho_control_interface`, allowing API consumers to distinguish RF CO2 control mode from regular operation

### Bug Fixes

- **`firmware.json` fetch silently failed** when the file grew past 2000 bytes, leaving `latest_fw` / `latest_beta_fw` empty and breaking the device's firmware update card. The cap is now derived from the actual `firmware.json` size at compile time with ample headroom, and overridable via the `MAX_FIRMWARE_HTTPS_RESPONSE_SIZE` build flag
- **`checkFirmwareUpdate()` had no error logging** on any failure path. Every step now emits an `E_LOG` line on failure (alloc, `https.begin`, `GET`, HTTP status, oversized response, JSON parse, missing `hw_rev` entry), making remote diagnosis possible without serial access
- **`firmware.json` `link` and `link_beta`** previously pointed at `raw.githubusercontent.com/.../master/...`, which 404'd whenever the binary wasn't yet on master. Both now use `https://github.com/arjenhiemstra/ithowifi/releases/download/Version-X.Y.Z/...` so binaries are reachable from the GH release directly
- **`build_script.py`** updated to write the same release-download URL form for new releases, so future builds get this right automatically
- **`WiFiClientSecure` leak** in `checkFirmwareUpdate()` — the allocation was leaked on every error path. All paths now `delete secclient` before returning

### API

- **NEW**: `GET /api/v2/ota`, `POST /api/v2/ota`
- **CHANGED**: `GET /api/v2/remotes` now also returns a `remotes` array (existing `remotesinfo` preserved)
- **CHANGED**: `GET /api/v2/deviceinfo` now includes `itho_control_interface`
- **UNCHANGED**: `POST /api/v2/debug` — the existing `update` / `update_beta` / `update_url` actions there continue to work and remain available; new integrations should prefer `/api/v2/ota`

### Web Interface

- New **RF standalone mode** radio in System Settings → CC1101 section
- New **wizard step** for RF standalone setup with Join button and live join-result feedback
- **Wizard info box** offering RF standalone when no I2C device is detected
- Update page redesigned with dropdown, install button, and embedded progress bar
- Standalone-mode menu visibility: I2C, virtual remotes, and status menus hidden when `itho_rf_standalone=1`

### Compatibility

This release is fully backwards-compatible with 3.1.0-beta1.

- All existing REST API endpoints continue to return the same fields they did before; `remotes` and `itho_control_interface` are additions, not replacements
- The legacy `POST /api/v2/debug` update actions and the WebSocket-based `update_url` flow are unchanged
- The HA integration version `0.4.0` of [ithowifi-ha-integration](https://github.com/arjenhiemstra/ithowifi-ha-integration) requires this firmware (3.1.0-beta2 or newer) to register the firmware update entity. Older firmware will simply omit the entity rather than fail

### Firmware binary

Firmware binary (CVE HW rev.2 and NON-CVE):
https://github.com/arjenhiemstra/ithowifi/releases/download/Version-3.1.0-beta2/nrgitho-v3.1.0-beta2.bin
