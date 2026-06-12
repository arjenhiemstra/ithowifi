# Version 3.1.5-beta2

Iterative beta on top of 3.1.5-beta1. One change: multi-zone HRUs (Itho DuoZone) now show up on the RF Status page instead of looking offline.

## Feat — multi-zone 31DA / 31D9 acceptance ([#366](https://github.com/arjenhiemstra/ithowifi/issues/366))

`parseRF31DA` and `parseRF31D9` dropped every frame whose first payload byte wasn't `0x00`. On Itho DuoZone HRUs (e.g. HRU ECO 300 R, model `03-00630`) the unit broadcasts the same opcode once per zone, with the zone ID in that byte (`0x15` / `0x16` / `0x17` for the typical living + sleeping + combined arrangement). All frames were therefore dropped and the source looked offline on `/api/v2/rfstatus`.

The guard is gone. The latest seen domain byte is now recorded on the `rfStatusSource` as `lastZone31DA` / `lastZone31D9` and surfaced as `zone31DA` / `zone31D9` in the `/api/v2/rfstatus` JSON output. Multi-zone consumers see the most recently broadcast zone's data rotating between zones; the new fields tell them which zone the current snapshot is from. Single-zone units stay at `0` and behave exactly as before.

Proper per-zone storage — separate measurements per zone surfaced simultaneously — is a follow-up. This release is the minimum unblock: from "no data" to "live data with a zone indicator".
