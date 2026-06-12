## Version 3.1.4

Rolls up 3.1.4-beta1 through beta7. Headline fixes: CVE ECO 2 SP fan control after a cold power cycle, the RF CO2 demand slider direction, the MQTT HA Discovery crash, and the memory / I2C stability issues that surfaced in long-running 3.1.3 installs.

### Bug Fixes

- **CVE ECO 2 SP slider no longer leaves the fan unresponsive after a cold power cycle** ([#346](https://github.com/arjenhiemstra/ithowifi/issues/346), [#364](https://github.com/arjenhiemstra/ithowifi/issues/364), [#365](https://github.com/arjenhiemstra/ithowifi/issues/365)) — `sendI2CPWMinit()` had been sending the CO2-init bytes (`0x13/0x13`) instead of the PWM-init bytes (`0x09/0x09`) since November 2024. First-generation CVE ECO 2 SP (`type 0x04 fw 0x01`) needs the correct frame to enter PWM-accept mode; modern CVE / HRU units accepted either. Boot-time init now fires the moment `QueryDevicetype` confirms a CVE unit, so affected boards recover on the first I2C handshake.
- **RF CO2 demand slider drives the unit in both directions and respects its current mode.** 3.1.3 always sent an `auto` command before each 31E0 demand, which the RFT CO2 unit treated as a boost — so downward slider moves were ignored when the unit was already in auto. The firmware now reads `FanInfo` (from I2C 31DA polling or a tracked RF 31DA source) and only sends `auto` when the unit isn't already in it. Affects the index slider, the REST `percentage` / `fandemand` endpoints, and the MQTT `cmd` topic.
- **MQTT Home Assistant Discovery no longer panics the device on heap exhaustion.** On devices with many status items (notably WPU, 134 items) the JSON build could hit `std::bad_alloc`; the uncaught exception fell to `std::terminate()` → reboot loop. The pre-flight check now guards on the largest contiguous block instead of total free, and the build is wrapped in `try / catch (std::bad_alloc)` so a mid-run OOM logs and skips.
- **Itho status no longer "sometimes empty until reboot".** `QueryStatusFormat` is retried once at boot if the first attempt returns zero entries, and partial / corrupt replies at runtime no longer wipe the previously good list. Mutex acquire timeout raised from 100 ms to 500 ms for the status / 31DA / 31D9 paths.

### Memory & Heap Stability

- WebSocket `systemstat` broadcasts skipped when no clients are connected — per-client allocations on every broadcast were a major source of long-term fragmentation.
- WebSocket broadcast bails when free heap drops below 80 KB; `notifyClients` uses `std::nothrow`.
- MQTT publish buffer is grow-only — the post-publish shrink-back was reallocating thousands of times per day.
- WebSocket queued-message cap reduced from 64 to 8.

### API

- **`GET /api/v2/speed` now also returns `timer_remaining_ms` and `timer_speed`** for add-on-tracked PWM2I2C timers ([#368](https://github.com/arjenhiemstra/ithowifi/issues/368)). Additive — older consumers ignore the new fields. HA integration v0.5.7+ surfaces this as "Add-on timer remaining".

### Web UI

- Spelling, grammar and capitalisation pass: typos fixed (`succesful` → `successful`, `normlized` → `normalized`, `satus` → `status`, `explaination` → `explanation`, `dataype` → `datatype`, `Github` → `GitHub`), brand and acronym capitalisation normalised (Itho, I2C, CVE, CO2, RV, RF, JSON, API, WiFi), page headers and menu items brought to sentence case, `ie.` → `e.g.` throughout.

### Firmware binary

https://github.com/arjenhiemstra/ithowifi/releases/download/Version-3.1.4/nrgitho-v3.1.4.bin
