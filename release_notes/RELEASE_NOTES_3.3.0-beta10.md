## Version 3.3.0-beta10

Tenth beta of the 3.3 line. Support for the old Itho RF protocol, RF signal diagnostics and several MQTT and RF fixes.

### Features

- **Old Itho RF protocol.** New remote type "RF (old protocol)" for units from before RAMSES, like the CVE ECO 2. As a send remote it joins and controls them with low, medium, high, timer 1-3, join and leave. These units send nothing back, so there is no status. To join, power the unit and send leave and then join within 2 minutes.
- **Old protocol capture.** The System log page has an "Old Itho RF protocol capture" section (Receive, Receive (loose), Off, Signal scan) that logs decoded frames as `RF1 rx` lines to the RF log, the system log and `<base topic>/debug/rf`. It switches off after 30 minutes. This helps to find out why a unit does not respond yet.
- **RF signal diagnostics.** Every RF debug line shows RSSI, LQI and frequency offset of that packet, and while RF logging is on an RF status (RFS) line with receive counters is added once per minute. "RF status in syslog (flash)" on the System log page also writes that line to the system log (#401, thanks @Borismism).
- **Ventilation demand of joined RF sensors.** A 31E0 from an RFT-RV or RFT-CO2 on a receive slot shows up in `remotesinfo` as `vent_demand` (0-200), `vent_demand_zone` and `vent_demand_flags` (#413).
- **Home Assistant discovery over mDNS.** The add-on announces `vendor`, `hwid` and `fw` in its mDNS info, which the HA integration (v0.9.0) uses to find add-ons on the network.

### Bugfixes

- **MQTT stream out of sync after a failed write.** When a publish could not be written completely, the connection stayed open and the broker read the next messages as part of the cut-off one. HA then received garbled payloads, and the add-on could end in a task watchdog reset. The add-on now closes the connection and reconnects (#415).
- **MQTT settings apply on save.** A changed server, port, login or base topic was only used after the connection dropped or after a reboot. The add-on now reconnects when they change. Saving unchanged settings keeps the connection.
- **Destination ID on a receive slot.** A slot saved as send and then changed to receive kept its RF ID until a reboot, so a join of that remote could store the unit's ID on the receive slot. That slot then showed no values. The ID is now cleared when the slot is saved as anything other than send (#410).
- **Leave of a send remote.** After a leave the slot kept the joined unit's ID and kept sending commands addressed to it, which the unit still follows. A leave now clears it and switches off the RF status request for that slot.
- **HA remote fan preset.** When the last command of a remote is not one of its presets (unknown, join, leave), HA showed an invalid preset and logged a warning on every update. It now shows no preset.
- **Old HA discovery topics.** Discovery topics of firmware 2.x stayed on the broker after an upgrade, which gave an extra fan entity and template errors in HA. They are now cleared.
- **Virtual remote last command.** A command the remote type does not have, like medium on a DemandFlow remote, no longer shows up as its last command.

### API

- **rfstatus by RF ID.** `GET /api/v2/rfstatus?id=AA:BB:CC` returns the tracked source with that RF ID. Names need not be unique, so the ID wins over the name when both are given.
- **RF remote index.** The REST rfremote command and config endpoints refused a remote in a slot above the number of used slots. All slots are accepted now.

### Firmware binary

https://github.com/arjenhiemstra/ithowifi/releases/download/Version-3.3.0-beta10/nrgitho-v3.3.0-beta10.bin
