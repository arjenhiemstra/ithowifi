## Version 3.2.0-beta1

First beta of the 3.2 line. Carries forward the work from the (skipped) 3.1.5-beta line and adds active RF status polling for RF-standalone setups.

### Features

- **Periodic 31DA + 31D9 RF status request in RF standalone mode.** When `itho_rf_standalone == 1`, the add-on now actively requests `31DA` and `31D9` from the Itho over RF on the same interval as the existing **Itho status update frequency** setting. Each request is addressed from the first bi-directional Send remote that's been joined to the unit, so an HRU 400 / RF-only setup can see live status without an I2C connection. Skipped silently when no qualifying remote is configured, when the CC1101 is absent, or when the update frequency is 0.
- **Debug page: Request 31DA / Request 31D9 buttons** with an RF remote selector and an optional 3-byte destination-ID override. Lets you probe an arbitrary Itho's RF address from an arbitrary slot — handy when a slot was never actually joined to a real unit. Override is one-shot: the slot's stored destinationID is snapshotted and restored after the request.
- **OTA progress over MQTT** so Home Assistant's firmware-update entity shows a progress bar instead of silently downloading and flashing ([#371](https://github.com/arjenhiemstra/ithowifi/issues/371)). The HA Discovery value template emits `in_progress` and `update_percentage`, and the firmware republishes `deviceinfo` ~once per second during OTA so the bar moves. No HA integration update needed — the new template ships in the discovery payload itself. End-to-end visibility kicks in on the *next* OTA after a 3.2.0-beta1 device republishes Discovery.
- **Multi-zone HRU support.** `parseRF31DA` / `parseRF31D9` accept any payload[0] "domain" byte instead of dropping everything non-zero, so Itho DuoZone HRUs no longer look offline on the RF Status page ([#366](https://github.com/arjenhiemstra/ithowifi/issues/366)). The latest seen domain byte is surfaced as `zone31DA` / `zone31D9` on `/api/v2/rfstatus`. Single-zone units stay at `0` and behave exactly as before. Proper per-zone storage is a follow-up.

### Bug fixes

- **`POST /api/v2/rfremote/config` with `setrfdevicedestid` now persists** to flash, not just the runtime CC1101 state. The value previously only lived in the in-memory CC1101 layer, so it was lost on reboot and on the next Update click on the RF Devices page.
- **destinationID survives the Update click on the RF Devices page.** The web-UI Update was calling `updateRFDevice`, which resets the in-memory `destinationID` to the slot's own sourceID and silently wiped anything a join handshake or the `setrfdevicedestid` API had put there. The persisted destID is now re-applied right after `updateRFDevice`.
- **Paired 31DA + 31D9 RF status request actually sends both opcodes.** Stuffing them into one ISR-disabled critical section left the CC1101 in an intermediate state for the second TX, so it silently didn't go out. Each frame now gets its own ISR cycle with a 50 ms gap.
- **Debug page buttons no longer reload the page.** Every button on the Debug page was inside a `<form>` with no `type="button"` attribute, so the browser treated each click as a form submit and navigated away before the websocket message could land. Marked them `type="button"`.
- **TX Power dropdown on the RF Devices page activates the correct slot.** The dropdown is only rendered for SEND remotes, so the `forEach` iteration index didn't match the slot index — selecting one Send remote enabled the previously-selected remote's TX Power dropdown.
