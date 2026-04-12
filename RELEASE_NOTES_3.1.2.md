## Version 3.1.2

Patch release hardening the device against crashes during OTA firmware updates.

### Bug Fixes

- **REST API returns 503 during active OTA download** — all REST endpoints (except `GET /api/v2/ota` for progress monitoring) now return `503 Service Unavailable` while a firmware download is in progress. This prevents any external poller (HA integration, MQTT scripts, browser tabs, other integrations) from triggering heap-exhaustion crashes during OTA. Previously, concurrent JSON serialization during OTA could cause `std::bad_alloc` → watchdog reset. The 503 gate lifts automatically once the download completes and the device reboots.

### Firmware binary

https://github.com/arjenhiemstra/ithowifi/releases/download/Version-3.1.2/nrgitho-v3.1.2.bin
