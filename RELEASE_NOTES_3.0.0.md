## Version 3.0.0

Major release with new features, new RESTful API, and significant improvements to RF communication.

### Highlights

- **RESTful API v2** — Proper REST API with GET/POST/PUT at `/api/v2/*`, OpenAPI spec, and Swagger UI
- **Bidirectional RF** — The add-on can now join as an RF remote (RFT CO2, RFT Auto-N, RFT-N, RFT RV, RFT Spider) and send commands directly to your Itho
- **RF CO2 demand control** — Ventilation demand slider (0–200) on the index page for CO2-based fan control
- **Setup wizard** — First-boot wizard for guided configuration
- **HA Auto Discovery** — RF devices, virtual remotes, fan component, Itho status items, and firmware update sensor
- **Security** — Encrypted credential exchange (AES-256-CTR), HTTP Basic Auth, CSRF protection, API parameter whitelisting
- **jQuery removed** — Full migration to vanilla JS (~30KB flash savings)

### Changes since 2.8.0

### Breaking Changes

- `/api.html` now always returns v1 plain text (OK/NOK); the query-parameter v2 style was removed
- `api_version` setting removed from system configuration
- REST API v2 at `/api/v2/*` is the new JSON API
- RF remote types auto-detect the bidirectional flag based on remote type

### New Features

- **Setup wizard** for first-boot configuration with step-by-step guidance
- **Encrypted credential exchange** (AES-256-CTR) between browser and device
- **RESTful API v2** with proper HTTP methods (GET/POST/PUT) at `/api/v2/*`
- **OpenAPI specification** and interactive Swagger UI at `/api/openapi.json` and `/swagger`
- **RF CO2 control interface** with demand slider (0–200) on the index page
- **RF CO2 join on boot** setting to auto-send join after power cycle
- **Bidirectional RF** for RFT-N, RFT Auto-N, RFT RV, RFT CO2, and RFT Spider remote types
- **RF binding initiator** — send join/bind requests from the add-on to pair with Itho units
- **Per-remote TX power** — configurable per RF device; auto-detected based on device type and I2C connection
- **Unified fan control** via `percentage` (0–100) and `fandemand` (0–200), available on REST, MQTT, and WebSocket; maps to RF CO2 demand or PWM speed depending on device type
- **MQTT command responses** published on `{base_topic}/cmd/response`
- **I2C status queries on boot** — all enabled queries (2401, 31DA, 31D9, 4210) run immediately after device init instead of waiting for their normal interval
- **RF status monitoring** with source tracking, custom names, and MQTT/Web API integration
- **HA Auto Discovery** for RF devices, virtual remote fan entities, Itho status items, fan component, and firmware update sensor (requires HA Core 2024.11+)
- **HA Auto Discovery State Class** as configurable option
- **CVE ECO2** device support in HA Auto Discovery
- **WPU firmware v41** status labels
- **Outside temperature API** for WPU devices
- **WPU manual control** (4030 command) via REST API
- **RFT-N remote type** support
- **Configurable number of RF remotes** as a system setting
- **Device info** extended on MQTT API and WebAPI
- **RF log** moved from debug page to syslog page with configurable log level
- **WebSerial** debug interface at `http://IP:8000/webserial`
- **Automatic firmware update check**
- **Changeable RF source ID** for the add-on
- **Raw 31D9 and 31DA** I2C message requests
- **HTTP Basic Auth** for API endpoints
- **get=vremotesinfo** command added to WebAPI v1
- **Option to reset HA Discovery config**
- **WiFi MAC address** in boot logging
- **Boot LED states** to visually confirm CC1101 module detection
- **api_reboot** setting to gate device reboot via API (default off)

### API

**REST API v2 Endpoints**
- **GET**: `/api/v2/speed`, `/ithostatus`, `/deviceinfo`, `/queue`, `/lastcmd`, `/remotes`, `/vremotes`, `/rfstatus`, `/settings`
- **POST**: `/api/v2/command` (with `percentage`/`fandemand`), `/vremote`, `/rfremote/command`, `/rfremote/co2`, `/rfremote/demand`, `/rfremote/config`, `/debug`, `/wpu/outside_temp`, `/wpu/manual_control`
- **PUT**: `/api/v2/settings`
- **CORS** support on all endpoints
- **Parameter whitelisting** on legacy `/api.html`

**MQTT**
- Command responses on `{base_topic}/cmd/response` with status, command, and timestamp
- `percentage`, `fandemand`, `rfco2`, `rfdemand`, `rfzone` keys supported
- Fix: vremote logic bug when both index and name provided (now `vremotename` takes precedence)
- Fix: MQTT cmd topic cleaned after successful command

### Web Interface

- **jQuery fully removed** — migrated to vanilla JS (~30KB flash savings)
- Setup wizard with device-specific defaults and instructions
- Swagger UI for interactive API exploration
- RF CO2 demand slider on index page with actual fan demand readback
- RF status monitoring page with tracked source data
- Per-remote TX power dropdown in RF Devices page (Send remotes only)
- WiFi status mapped to user-friendly text (Connected, Disconnected, etc.)
- Hostname moved above Advanced settings in WiFi wizard
- Auto-save remote config before entering learn/leave mode
- Auto-generate remote ID when switching to Send mode
- Remote capabilities inputs disabled until remote is selected
- DemandFlow/QualityFlow wizard defaults (12 virtual remotes, RFT DF/QF type)

### RF / CC1101

- Bidirectional RF communication with full bind handshake (offer → accept → confirm)
- Split `receivePacket` into ISR read and deferred decode with double buffering
- End byte selection based on decoded message length parity (even → 0xAC, odd → 0xCA)
- Per-remote TX power with auto-detect: CVE/HRU200 at −30 dBm, other I2C at +5 dBm, standalone at +10 dBm
- Source ID set per Send remote at boot and on config update
- Binding accept matched against source ID (not just module default ID)
- RF CO2 send functions: `send1298` (CO2 ppm), `send12A0` (temp/humidity), `send31E0` (demand), `sendRQ31DA` (status request)
- 200ms delay between auto and demand RF commands to prevent collisions
- CC1101 SPI lockup prevention with chip version check
- RF ISR reentry prevention with mutex
- Wizard defaults to RFT CO2 for non-PWM2I2C HRU devices (HRU350, HRU250-300, HRU ECO)

### Bug Fixes

- Fix: coredump download crashed device (use-after-free in chunked response) *(beta2)*
- Fix: WebSocket buffer race condition causing heap corruption *(beta2)*
- Fix: OTA static buffer overflow on repeated updates *(beta2)*
- Fix: I2C safe guard null pointer dereference *(beta2)*
- Fix: uninitialized decrypt buffer *(beta2)*
- Fix: strncpy replaced with strlcpy throughout codebase *(beta2)*
- Fix: force medium doesn't work when virtual remote is not RFTCVE, RFTCO2, or RFTRV type
- Fix: duplicate utc-time key for WPU
- Fix: remote learn/leave mode not deactivated after timer runs out
- Fix: web UI hex remote ID entry with values < 0x10
- Fix: rounding issues from double-to-float migration
- Fix: remotes received but not processed
- Fix: config not saved when done in a certain order [#294](https://github.com/arjenhiemstra/ithowifi/issues/294)
- Fix: settings not updated in Itho Settings dialog [#310](https://github.com/arjenhiemstra/ithowifi/issues/310)
- Fix: setting not updated on 1st try from browser cache [#310](https://github.com/arjenhiemstra/ithowifi/issues/310)
- Fix: Itho settings via API causes reboot [#316](https://github.com/arjenhiemstra/ithowifi/issues/316)
- Fix: web socket parsing issues
- Fix: mDNS hostname lowercase only
- Fix: config save before load protection
- Fix: HA Discovery support for larger config files (8K)
- Fix: websocket receive refactored for multi-frame large messages
- Fix: setrfdevicesourceid API call handling
- Fix: favicon URL
- Fix: factory reset uses delayed reboot instead of immediate esp_restart
- Fix: encrypted credential decryption bypassing password filter
- Fix: password masking in presentation layer (not in config get)
- Fix: empty remote slots properly identified and skipped
- Fix: WebSocket message handlers null-check DOM elements
- Fix: device type check for pwm2i2c support (|| vs && logic error)
- Fix: empty API request returns fail instead of error
- Fix: operator precedence bug in IthoSettings.cpp
- Fix: API error messages now list valid options instead of generic "cmdvalue not valid"
- Fix: end byte encoding based on decoded message length parity
- Fix: binding accept matched against source ID (not just default module ID)
- Fix: auto-save remote config before entering learn/leave mode
- Fix: MQTT crash when rfremoteindex sent as JSON integer [#350](https://github.com/arjenhiemstra/ithowifi/issues/350)
- Fix: auto-fill module RF ID when switching remote to Send mode [#352](https://github.com/arjenhiemstra/ithowifi/issues/352)
- Fix: Itho status I2C mutex to prevent bus lockup under high fan demand
- Fix: wizard settings not saved (ArduinoJson v7 string-to-int conversion)
- Fix: wizard systemsettings response overwriting user-configured form fields
- Fix: wizard RF remote config not persisted (shared DelayedSave ticker race)
- Fix: wizard RF remote type read from global dropdown instead of per-remote selection
- Fix: wizard state not preserved across WiFi reboot on first boot
- Fix: wizard WiFi status flashing "Disconnected" instead of "Connecting..."
- Fix: Send remote buttons routed to virtual remote I2C path instead of RF command path
- Fix: per-remote TX power ignored — hardcoded low power used for all Send remotes
- Fix: OTA progress percentage exceeding 100%
- Skip I2C check for WPU devices

### Security

- AES-256-CTR encrypted credential exchange (SecureWebCommLite)
- HTTP Basic Auth on all REST API v2 endpoints
- Parameter whitelisting on legacy `/api.html`
- Reboot via API gated by `api_reboot` setting (default off)
- CSRF inherently blocked on REST API (POST/PUT require `Content-Type: application/json`)
- WebSocket buffer mutex for thread-safe concurrent access *(beta2)*
- I2C mutex for Itho status queries to prevent bus lockup

### Build & Dependencies

- jQuery removed
- Switched from stale ESPAsyncWebServer to maintained ESP32Async repos
- ArduinoJson updated to v7.4.3
- ArduinoStreamUtils updated to v1.9.2
- AsyncTCP v3.4.10
- ESPAsyncWebServer v3.10.0
- Switched to PubSubClient3
- Mongoose removed
- Build flags for C (gnu17) / C++ (gnu20)

### Testing

- 304 native unit tests on x86 (PlatformIO native_test environment)
- 250+ integration tests (Python/pytest against live device)
- MQTT integration tests with broker authentication
- Schemathesis OpenAPI fuzzing support
- Test coverage for: config serialization, JSON parsing, speed/timer validation, API parameter validation, queue logic, remote management, RF logic, UUID, WiFi config, MQTT responses

### Documentation

- OpenAPI 3.0 specification at `/api/openapi.json` and as standalone file at `docs/openapi.json`
- Interactive Swagger UI at `/swagger`
- API documentation page with REST API v2 examples, MQTT reference, and full parameter table
- Web UI text updated for remotes, virtual remotes, and status pages

### Firmware binary

Firmware binary (CVE HW rev.2 and NON-CVE):
https://github.com/arjenhiemstra/ithowifi/releases/download/Version-3.0.0/nrgitho-v3.0.0.bin
