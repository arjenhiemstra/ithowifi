# Version 3.1.4-beta6

Iterative beta on top of 3.1.4-beta5. Fixes the root cause of [#365](https://github.com/arjenhiemstra/ithowifi/issues/365) (CVE ECO 2 SP slider not driving the fan after a cold power-cycle) and tidies up the web UI text.

## Fix — PWM-init payload restored (#365)

`sendI2CPWMinit()` had been sending the wrong frame since commit [1efa17e](https://github.com/arjenhiemstra/ithowifi/commit/1efa17e) (Nov 2024). The two init helpers (PWM-init and CO2-init) share the same frame layout and differ only in bytes 8 and 10 — `0x09/0x09` for PWM, `0x13/0x13` for CO2. A copy/paste while adding the new `sendCO2init()` helper next to it pasted the CO2 bytes into the PWM frame. Every release from then on shipped the wrong payload.

CVE/HRU200 units other than the first-generation CVE ECO 2 SP silently accept either frame, so the bug stayed invisible. The CVE ECO 2 SP (`type 0x04 fw 0x01`) needs the correct `0x09/0x09` PWM-init before it will act on subsequent PWM speed writes, so after a cold power-cycle the slider would move but the fan would not respond. Downgrading to 2.8.0 / 2.8.1 worked because those releases predate the regression.

The fix is the one-line revert in `IthoVirtualRemoteCmd.cpp::sendI2CPWMinit()`. Combined with the boot-time `sendI2CPWMinit()` call added in beta3, affected units now recover automatically on the first I2C handshake — no manual intervention.

## Web UI — typo and capitalisation pass

Spelling fixes (`succesful` → `successful`, `normlized` → `normalized`, `satus` → `status`, `explaination` → `explanation`, `dataype` → `datatype`, `Github` → `GitHub`, `to you Itho` → `to your Itho`, a few `a RF/MQTT` → `an RF/MQTT`, and the broken "Command topicmands are sent as JSON to the MQTT com" sentence on the API page), plus the `ttype="text"` attribute typo on the debug page.

Normalised brand / acronym capitalisation across the UI (`Itho`, `I2C`, `CVE`, `CO2`, `RV`, `RF`, `JSON`, `API`, `WiFi`) and brought all page headers and menu items to sentence case so the setup wizard matches the rest of the UI. Also replaced `ie.` with `e.g.` everywhere — every use was an example, not a clarification.
