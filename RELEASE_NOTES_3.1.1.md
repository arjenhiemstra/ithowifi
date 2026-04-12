## Version 3.1.1

> **⚠️ This version is superseded by [3.1.2](https://github.com/arjenhiemstra/ithowifi/releases/tag/Version-3.1.2).** Version 3.1.2 adds a server-side 503 gate during OTA that protects against all external pollers, not just the HA integration. Please upgrade to 3.1.2.

Patch release fixing a crash during OTA firmware updates when REST API requests arrive concurrently.

### Bug Fixes

- **Device crash during OTA when REST API is polled** — serializing a JSON response (e.g. `GET /api/v2/remotes`) while an OTA download is in progress could exhaust the heap, causing `std::bad_alloc` → `abort()` → watchdog reset on the `async_tcp` task. The `sendSuccess` / `sendFail` / `sendError` helpers now catch allocation failures and return a `503 Service Unavailable` plain-text response instead of crashing.

### Firmware binary

https://github.com/arjenhiemstra/ithowifi/releases/download/Version-3.1.1/nrgitho-v3.1.1.bin
