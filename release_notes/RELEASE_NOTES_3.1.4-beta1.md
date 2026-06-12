# Version 3.1.4-beta1

This is a beta release focused on stability fixes that surfaced after 3.1.3 went out. Most users should be safe to install; users on WPU units in particular benefit. Please report back via GitHub issues if you see anything unexpected.

## Highlights

- **Memory stability**: long-running units no longer suffer slow heap erosion that eventually triggers `SDIO_RESET` after a few days of uptime. Verified on a CVE-Silent test unit running ~14 days continuously without reset since the fixes were applied.
- **WPU status reliability**: addresses the "status sometimes there, sometimes not" complaint reported with 3.1.3. A boot-time `QueryStatusFormat` race could leave the device in a state where the status board never populated until the next reboot. Now retried automatically and a partial / corrupt I2C reply no longer wipes a previously-good status list.

## Bug fixes

### Memory & heap

- Skip WebSocket `systemstat` broadcasts when no clients are connected — every broadcast otherwise allocated a per-client buffer + deque node, accumulating fragmentation over days even when nobody was listening.
- WebSocket broadcast now bails out when free heap drops below 80 KB, preventing fragmentation spirals during transient memory pressure.
- `notifyClients` switched to `std::nothrow` allocation so a failed allocation degrades gracefully instead of throwing.
- MQTT publish buffer is now grow-only — repeated realloc cycles between the default and a larger size were a major source of long-term heap fragmentation.
- HA Discovery publish skipped when free heap is below 100 KB. Fixes a crash path where an already-low-heap device cascaded into `SDIO_RESET` while building the (large) discovery JSON.
- WebSocket queued-message cap reduced from 64 to 8.
- REST `503` log now includes free heap and largest-block size for diagnosis.

### I2C status reliability

- `QueryStatusFormat` is retried once at boot if the first attempt returned 0 entries. Previously a transient mutex-contention or partial I2C reply at boot would leave the status board permanently empty until the user rebooted manually.
- Status-format parsing now happens into a local list and is only swapped into the live state if the new reply has at least one entry. A 0-length / corrupt reply at runtime no longer wipes the previous (good) data.
- Same defensive 0-length check added to the 31DA and 31D9 paths.
- Mutex acquire timeout raised from 100 ms to 500 ms for the `QueryStatusFormat`, `QueryStatus`, `Query31DA`, and `Query31D9` paths. Failure to acquire now logs a warning instead of silently dropping the update.

### Cosmetic / diagnostic

- The `RF: TX power 0x... for Send remotes` log line and the corresponding remote-flash write are now skipped on units without a CC1101 module — the value was meaningless and the log line was confusing on I2C-only setups.

## Known issues / not addressed in this release

- CVE ECO 2 SP fan control after upgrade ([#364](https://github.com/arjenhiemstra/ithowifi/issues/364) / [#365](https://github.com/arjenhiemstra/ithowifi/issues/365)): if your fan does not respond to the speed slider, please confirm **System Settings → Enable/Disable I2C commands → CVE fan control (PWM2I2C)** is set to **on**. We are still investigating whether anything in the firmware needs to change beyond this user setting.

## How to test

If you upgrade and notice anything off, please open a GitHub issue with:
- Your `/api/v2/deviceinfo` output
- The first ~50 lines of the System Log after a fresh boot (so we can see `QueryDevicetype`, `QueryStatusFormat - items:N`, etc.)
- A description of what you expected vs. what you saw
