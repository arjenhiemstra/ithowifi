## Version 3.3.0-beta3

Third beta of the 3.3 line. Adds Itho Spider thermostat support and a selectable RF status poll, fixes a random crash on add-ons with an RF module and status values that froze until a reboot.

### Features

- **Itho Spider room thermostats over RF.** The add-on can read an Itho Spider's room temperature, setpoint and heat demand, and set its setpoint. A wizard on the RF remotes page finds the Spider and the heater or relay it is bound to; the Spider stays bound to that heater or relay. A Spider only listens now and then, so a new setpoint can take a few minutes to apply; touching the Spider's display applies it at once.
- **Setpoint via MQTT and REST.** MQTT `{"rfsetpoint": 20.5, "rfremoteindex": <n>}` and `POST /api/v2/rfremote/setpoint`. The values are in the remote's capabilities (`/api/v2/remotes`, MQTT `remotesinfo`). The HA integration v0.7.0 shows a Spider as a `climate` entity.
- **RF status poll from any send remote.** Choose a bi-directional send remote (for example an RFT Spider) on the new **RF settings** page, and the add-on requests the fan status over RF at the status update interval. The CC1101 settings moved from System settings to that page.
- **WiFi security shown on the setup page.** The WiFi setup page shows the negotiated security (WPA2/WPA3).
- **Resizable RF log.** The RF log box can be resized.

### Bugfixes

- **Fixed a random crash on add-ons with an RF module.** An RF packet arriving while the add-on was writing to flash could crash and reboot it. The packet is now read outside the interrupt.
- **Temperature and humidity no longer freeze until a reboot (#295).** One out-of-range reading switched the field to "not available" for good. It now recovers on the next valid reading. Same for the highest CO2 and RH values. Thanks to @polroskam (#394).
- **Bi-directional remotes joined to the add-on get a reply.** An RFT AUTO-N showed a red LED and repeated each command, and an RFT CO2 showed a join error. The add-on now answers like the fan does.
- **RF remotes after a gap in the remote list.** A remote after an empty slot was registered in the wrong internal slot, so its type and bi-directional setting were not used when sending. Removing a remote, in the web interface or with a leave command, could also leave it active or clear another remote until a reboot.
- **Learn/leave mode saves the RF config once.** Without a selected remote the config was saved once per row.
- **RF remote IDs are shown with two digits per byte** (`96,E6,07` instead of `96,E6,7`).
- **WPA3 networks are shown as secured** in the WiFi scan list.
- **RF frames with a bad checksum are no longer acted on.**
- **31D9 status flags are decoded correctly** on the wired (I2C) path.
- **The log shows the real reset reason.** A crash was logged as `OWDT_RESET`; it is now logged as `PANIC_RESET`.

### Performance

- **Faster RF frame decoding and encoding** (table driven instead of bit by bit).

### Firmware binary

https://github.com/arjenhiemstra/ithowifi/releases/download/Version-3.3.0-beta3/nrgitho-v3.3.0-beta3.bin
