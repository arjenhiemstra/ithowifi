# Version 3.1.5-beta1

First iteration beyond 3.1.4 stable. One user-visible change so far: Home Assistant's firmware update button now shows progress.

## Feat — OTA progress visible in Home Assistant ([#371](https://github.com/arjenhiemstra/ithowifi/issues/371))

Clicking the firmware "Install" button in HA used to silently download and flash with no visible feedback until the device rebooted on the new version about two minutes later. HA's MQTT Update integration renders a progress bar when the value template emits `in_progress` and `update_percentage`, but the firmware-update discovery entity only emitted `installed_version` / `latest_version`.

This release:

- Adds an `ota_progress` integer (`-1` idle, `0-100` active, `101` done, `-2` error) to the MQTT `deviceinfo` topic.
- Updates the HA Discovery value template to derive `in_progress` and `update_percentage` from `ota_progress`.
- Republishes `deviceinfo` whenever `ota_progress` crosses idle ↔ active, otherwise rate-limited to once per second, so the progress bar moves smoothly instead of waiting on the periodic status tick.

No HA integration update needed — the new template is in the discovery payload itself. After this firmware boots once, HA picks up the updated entity definition on the next discovery republish.
