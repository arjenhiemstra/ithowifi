## Version 3.3.0

Stable cut of the 3.3 line, the sum of twelve betas. Itho Spider thermostats over RF, Orcon RF support, the old Itho RF protocol, Air to Water Heatpump support, up to 20 remotes, much more free memory and a long list of RF, MQTT and Home Assistant fixes.

**Note for Home Assistant users coming from 3.2.0:** the fan, firmware update and per remote entities get their ID from the hostname (#387). Home Assistant recreates these entities once after the update; changes you made to them in Home Assistant (name, area, entity ID) have to be redone. Discovery topics left behind by firmware 2.x are cleared.

### Features

**RF: Itho Spider thermostats**

- **Read and set Itho Spider room thermostats over RF.** Room temperature, setpoint and heat demand are read, the setpoint can be set. A wizard on the RF devices page finds the Spider and the heater, relay or AutoTemp zone it is bound to. Setpoint via MQTT `{"rfsetpoint": 20.5, "rfremoteindex": <n>}` and `POST /api/v2/rfremote/setpoint`; the values are in `/api/v2/remotes` and MQTT `remotesinfo`. The HA integration shows a Spider as a `climate` entity.
- **Bind as gateway.** The add-on can take the place of an Itho gateway for a Spider: it accepts the Spider's pairing offer, answers its polls and dial change reports, and a new setpoint also shows on the Spider's display. Meant for Spiders on an AutoTemp, where the Spider otherwise re-sends its own setpoint. A setpoint change on the Spider itself is kept.

**RF: more devices and protocols**

- **Orcon 15RF and Orcon CO2 remote types.** Control an Orcon MVS-15 over RF as a 15RF remote, and send CO2 demand as an Orcon CO2 sensor.
- **Old Itho RF protocol.** Remote types "RF (old protocol)" and "RF (old protocol, variant 2)" for older units like the CVE ECO 2: join, leave, low, medium, high and the timers. These units send nothing back. To join, power the unit and send leave and then join within 2 minutes. A capture mode on the System log page logs the decoded frames. Compared with the betas the checksum is corrected: with beta10 and beta11 half of all remote IDs were ignored by the unit.
- **Periodic RF CO2 status request and keep-alive.** With RF CO2 control, the add-on can request 31DA and 31D9 from the Itho at the status interval and periodically re-send the last fan demand (31E0) and CO2 level (1298).
- **RF status request from any bi-directional send remote**, chosen on the new RF settings page, where the CC1101 settings now live. After a successful join of a bi-directional send remote the status request switches on for that remote.
- **Up to 20 RF and virtual remotes** (was 12). See "Going back to an older firmware" below.
- **Join from a selectable virtual remote** for "Send join command: next power on" (#397).
- **Ventilation demand of joined RF sensors.** A 31E0 from an RFT-RV or RFT-CO2 on a receive slot shows up in `remotesinfo` as `vent_demand`, `vent_demand_zone` and `vent_demand_flags` (#413).
- **Multi-zone.** The 31DA and 31D9 MQTT messages carry the zone of their values (#383).

**Wired devices**

- **Itho Air to Water Heatpump** (AWHeatpump) with settings and status labels for firmware 1 and 2.
- **Device labels checked and regenerated:** fixed CVE-Silent fw 9 settings from index 26, HRU ECO-fan fw 2 to 8, AutoTemp fw 3 and 4, CVE fw 1 status labels; new HRU 350 fw 5 and DemandFlow fw 5 settings. Status label keys (MQTT, HA) are unchanged.
- **I2C debug page reworked.** Serial number, article number, heat pump faults, identify, start deaeration, a generic I2C message sender, device type with name and versions, and a fault history with fault texts per device.
- **I2C debug and sniffer as toggles** with status; the sniffer switches without a reboot.

**Diagnostics**

- **RF signal diagnostics.** Every RF log line shows RSSI, LQI and frequency offset, and an RFS line with receive counters is added once per minute, optionally also to the system log (#401).
- **RF log and I2C sniffer on MQTT** at `<base>/debug/rf` and `<base>/debug/i2c` while they are on.
- **Device info** has `heap_free`, `heap_min_free` and `heap_max_block`; the RF log box is resizable.

**Other**

- **Automatic firmware update check can be turned off** (System settings, System security) for add-ons without internet access.
- **Home Assistant discovery over mDNS.** The add-on announces `vendor`, `hwid` and `fw`, which the HA integration uses to find add-ons on the network.
- **WiFi setup page** shows the negotiated security (WPA2/WPA3); WPA3 networks show as secured in the scan list.

### Memory and stability

- **More free memory.** On a CVE Silent, free memory went from about 75 KB to 100 KB and the lowest point after boot from about 24 KB to 63 KB: smaller TLS and MQTT buffers, constant tables in flash, finished tasks give their memory back, REST replies and MQTT sends no longer hold extra copies. The free memory shown is now the memory that can be used, so the number is lower for the same device.
- **Crash fixes.** An RF packet arriving during a flash write (the packet is now read outside the interrupt); a status read with an uninitialised field; a use-after-free of the TLS client at the end of the firmware check; the web server aborting on low memory (it now answers 503); a data race on the RF status vectors.
- **RF task under the watchdog.** After a reported RF dropout ("CC1101 Task timed out!" and silence): every wait on the radio has a time limit, the RF task is watched again, and the receive is restarted when the chip stops receiving.
- **I2C bus shared between tasks.** Only queued I2C commands ran one after the other; the web, MQTT and RF tasks also used the bus directly, and a transfer that collided with the status poll failed with `i2cMasterInit error: ESP_FAIL` or was lost silently. All transfers now take one lock (thanks @thijsdejong). A stuck bus that could not be cleared was still written to; a failed bus check now aborts the transfer.
- **RF module not found at boot (#400):** detection is retried after a chip reset and at a lower SPI speed.
- **Reboot after a manual firmware upload** hung for a minute with RF status requests on. A slow upload failed completely.
- **Logging stopped for good** after a log queue overflow.

### Bug fixes

**RF**

- RF remote ID lost when changing a remote's function (#376); remotes after a gap in the list registered in the wrong slot; removing a remote could leave it active or clear another one until a reboot.
- Stale destination IDs: after removing a remote (#410), after saving a slot as receive, and after a leave (the unit kept following commands addressed from the old ID).
- A 31D9 or 31DA frame from a neighbouring unit could be filed under your own device (#406).
- Bi-directional remotes joined to the add-on get a reply like the fan gives, so an RFT AUTO-N no longer shows a red LED and an RFT CO2 no longer reports a join error.
- RF frames with a bad checksum are no longer acted on; 31D9 status flags decoded as a bitmap on the wired path.
- Filter, fault and speed status over RF in RF CO2 mode requested again; the joined RF CO2 remote is selected automatically.
- Remote buttons on the virtual remotes and RF devices pages ran the command twice.
- The REST rfremote command and config endpoints accept every slot, not only slots below the number of used slots.

**Wired and status**

- Temperature and humidity no longer freeze until a reboot after one out-of-range reading; same for the highest CO2 and RH values (#295, thanks @polroskam).
- Shifted status values with the I2C sniffer on: the sniffer interrupt is off while the add-on receives a reply (#295, #403).
- Itho status empty after a power event: the status format query is retried until it succeeds (#409).
- REST settings return HTTP 503 when the I2C command fails (#390); `GET /reset` answers 200 (#409).

**Configuration and web interface**

- Settings could not be saved after a broken config file: files are written via a temporary file and an unreadable file is replaced by the defaults.
- Remotes gone after a reboot when the remotes file grew past 4000 bytes: the file only holds the configuration now.
- Saving the WiFi settings failed with "Unable to write config file"; with this fixed a WPA3-only network works (#303).
- RF remote IDs shown with two digits per byte; learn/leave mode saves the config once; the log shows the real reset reason (`PANIC_RESET` instead of `OWDT_RESET`); accessible form controls.

**MQTT and Home Assistant**

- MQTT stream out of sync after a failed publish write, which gave garbled payloads in HA and could end in a watchdog reset: the connection is closed and reopened (#415).
- Changed MQTT settings apply on save instead of after the next disconnect.
- HA entity IDs clashed between two add-ons of the same type (#387); fan presets for CVE and HRU 200 with PWM2I2C (#384); HRU-eco speed looked up by label (#388); template error on an empty last command (#392); invalid preset warning when the last command is not a preset; a command the remote type does not have is not shown as its last command.
- Virtual remotes no longer offer the bi-directional models, they do not work as a virtual remote (#405). Already configured bi-directional remotes are not touched.

### API

- `GET /api/v2/rfstatus?id=AA:BB:CC` returns the tracked source with that RF ID.
- `POST /api/v2/rfremote/setpoint` and MQTT `rfsetpoint` for Spider thermostats.
- `vent_demand`, `vent_demand_zone` and `vent_demand_flags` in `remotesinfo` (#413); `zone` in the 31DA and 31D9 MQTT messages (#383); heap fields in `deviceinfo`.

### Performance

- Faster RF: CC1101 SPI at 4 MHz with block transfers, table driven RF frame decoding and encoding.
- Smaller web interface: the JavaScript is minified at build time.
- Libraries: AsyncTCP 3.5.0, ESPAsyncWebServer 3.12.1, MycilaWebSerial 8.2.3, pubsubclient3 3.3.2.

### Going back to an older firmware

A setting above 12 remotes is safe on older firmware, it then uses 12 slots. Remotes above slot 12 are not available there, and are lost if that firmware saves the remotes file.

### Firmware binary

https://github.com/arjenhiemstra/ithowifi/releases/download/Version-3.3.0/nrgitho-v3.3.0.bin
