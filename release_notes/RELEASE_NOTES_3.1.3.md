## Version 3.1.3

Latest stable release. Supersedes all previous 3.1.x releases.

### Bug Fixes

- **"Retrieve settings" triggered an accidental OTA firmware reinstall** ([#359](https://github.com/arjenhiemstra/ithowifi/issues/359)) — the websocket message `{"ithogetsetting": true, "index": 0, "update": false}` contained an `"update"` key with a boolean value. The websocket and MQTT handlers only checked `isNull()` on the `"update"` key, not its type, so the boolean `false` was interpreted as a string and triggered `triggerOTAUpdate()`. Fixed by adding a `is<const char *>()` type check so only string values (`"stable"`, `"beta"`, `"auto"`) are accepted as OTA commands.
- **JavaScript console error: Maximum call stack size exceeded on `messageHandlers.systemstat`** — the Update page's inline script wrapped the `systemstat` handler with a decorator every time the page was loaded. Navigating to the Update page multiple times built up recursive layers until the call stack overflowed. Fixed by guarding the decorator with a one-time flag.

### Firmware binary

https://github.com/arjenhiemstra/ithowifi/releases/download/Version-3.1.3/nrgitho-v3.1.3.bin
