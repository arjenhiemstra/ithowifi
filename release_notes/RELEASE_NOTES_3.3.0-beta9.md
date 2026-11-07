## Version 3.3.0-beta9

Ninth beta of the 3.3 line. Three fixes on top of beta8.

### Features

- **Zone on the 31D9 MQTT message.** The 31D9 RF status message now has `zone`, like the 31DA message. It is the zone of the frame the published values came from (#383).

### Bugfixes

- **Itho status empty after a power event.** If the status format query returned 0 items at boot, the status stayed empty until a reboot. It is now retried every 60 s until it succeeds. A failing retry is logged once (#409).
- **Stale RF destination ID.** Removing a remote did not clear its destination ID, so a remote added later in that slot got it too. A receive remote with a foreign destination ID shows no values from its own sensor. Removing now clears it, and an empty slot that still has one is cleaned at boot (#410).
- **Reboot over HTTP.** `GET /reset` answered 501 although the device rebooted. It now answers 200 (#409).

### Firmware binary

https://github.com/arjenhiemstra/ithowifi/releases/download/Version-3.3.0-beta9/nrgitho-v3.3.0-beta9.bin
