## Version 3.1.0-beta3

Fixes the Home Assistant MQTT-discovery firmware-update entity for beta-channel users, adds per-remote `last_cmd` tracking so external integrations can read the last command dispatched via each remote, and fixes a silent bug in `GET /api/v2/vremotes` that caused it to return an empty response.

**⚠️ This is a beta release — please report issues at https://github.com/arjenhiemstra/ithowifi/issues**

### Highlights

- **Channel-aware HA Discovery firmware update entity** — no more bogus "update available" notifications for users running a beta, no more empty release URLs when `firmware.json` hasn't been fetched yet
- **Per-remote `last_cmd` field** in `GET /api/v2/remotes` and `GET /api/v2/vremotes`, populated when an RF SEND remote or virtual remote dispatches a user-facing fan command (low / medium / high / auto / timer1-3 / cook30 / cook60 / away / autonight / etc.)
- **`GET /api/v2/vremotes` now actually returns data** — previously silently returned `{}` due to a `JsonDocument::as<>()` vs `to<>()` bug

### New Features

- **`add-on_update_channel` and `add-on_latest_for_channel`** added to `GET /api/v2/deviceinfo`. The channel is auto-detected from the running firmware version: a pre-release string (`-beta` / `-rc` / `-alpha` / `-dev`) pins the device to the beta channel, otherwise stable. `add-on_latest_for_channel` is clamped to the installed version if no newer firmware is available on the active channel — this means HA's update entity stays quiet for beta users running the latest beta, and never shows an empty `latest_version` if `firmware.json` hasn't been fetched.
- **HA Auto Discovery update entity** value template updated to use `add-on_latest_for_channel` for both `latest_version` and the GitHub `release_url`. The `release_url` is now computed dynamically from the template instead of baked in at discovery publish time.
- **`payload_install` is now `{"update":"auto"}`** — resolves on the device to stable or beta based on the running version, so the install button always installs the channel matching what the user is currently on. No more accidental downgrades for beta users.
- **MQTT command `{"update":"auto"}`** — new value accepted on the command topic, equivalent behavior to the new `payload_install`. The existing `"stable"` and `"beta"` values continue to work unchanged.
- **`last_cmd` field on RF SEND remotes** — populated in `ithoExecRFCommand()` after a successful dispatch, only for remotes with function `SEND`. Visible in the new `remotes` array of `GET /api/v2/remotes`.
- **`last_cmd` field on virtual remotes** — populated in `sendRemoteCmd()` (the chokepoint for all vremote command dispatch). Visible in `GET /api/v2/vremotes` response under `vremotesinfo[*].last_cmd`.
- **New helpers in `IthoRemote`**: `setLastCmd(index, cmd)`, `getLastCmd(index)`, and `static ithoCommandToShortName(code)` for converting `IthoCommand` enum values to their short lowercase API names.

### Bug Fixes

- **`GET /api/v2/vremotes` returned an empty object.** The handler used `JsonDocument::as<JsonObject>()` on a fresh document, which returns a null `JsonObject`, so `virtualRemotes.get(obj, "vremotesinfo")` silently wrote into a dead object. Fixed to use `data.to<JsonObject>()` to match the sibling `handleGetRemotes`. Any caller of `/api/v2/vremotes` now gets the actual virtual remote configuration.
- **Beta user on `3.1.0-beta2` saw bogus "update to 3.0.0" notification in HA** (issue #358). Fixed by the new channel-aware logic above.
- **Empty `release_url` in HA** when `firmware.json` hadn't been fetched (issue #358) — the link ended at `.../releases/tag/Version-` with nothing after it. Fixed by clamping `add-on_latest_for_channel` to the installed version when no newer version is known.

### API

- **CHANGED**: `GET /api/v2/deviceinfo` — new fields `add-on_update_channel` and `add-on_latest_for_channel`
- **CHANGED**: `GET /api/v2/remotes` — `remotes` array entries now include `last_cmd` when a command has been dispatched via a SEND remote
- **CHANGED**: `GET /api/v2/vremotes` — now actually returns the `vremotesinfo` array (was returning empty), entries now include `last_cmd` when a command has been dispatched
- **CHANGED**: MQTT command topic — accepts `{"update":"auto"}` in addition to `"stable"` / `"beta"`
- **UNCHANGED**: `remotesinfo` shape (used by MQTT publish, legacy WebAPIv1, and the `remotesinfo` field of `GET /api/v2/remotes`) — fully backwards-compatible. The existing `capabilities.lastcmd` / `capabilities.lastcmdmsg` fields on RX remotes are untouched

### Compatibility

Fully backwards-compatible with 3.1.0-beta2.

- Existing REST fields all still present with their previous shapes; new fields are additive.
- Existing MQTT discovery consumers see a firmware update entity that behaves correctly for the first time if they're on a beta, and unchanged if they're on stable.
- The `POST /api/v2/debug` update actions (`update`, `update_beta`, `update_url`) are unchanged.
- `capabilities.lastcmdmsg` on RX remotes (used by HA discovery templates) is untouched.

### Closes

- [#358](https://github.com/arjenhiemstra/ithowifi/issues/358) — Wrong URL in release notes link for firmware update (Home Assistant)

### Firmware binary

Firmware binary (CVE HW rev.2 and NON-CVE):
https://github.com/arjenhiemstra/ithowifi/releases/download/Version-3.1.0-beta3/nrgitho-v3.1.0-beta3.bin
