## Version 3.3.0-beta8

Eighth beta of the 3.3 line. Much more free memory, several crash fixes, up to 20 remotes and RF log and I2C sniffer output over MQTT.

### Features

- **Up to 20 RF and virtual remotes.** Some Itho devices hold up to 20 remotes, so the limit of 12 is gone. See the note on going back to an older firmware below.
- **RF status request after a join.** After a successful join of a bi-directional send remote, the periodic RF status request switches on for that remote. A device like an HRU only reports its status when asked. You can still switch it off. An existing join does not change, switch it on yourself in the RF settings.
- **RF log and I2C sniffer on MQTT.** While the RF log or the I2C sniffer is on, each line is also published to `<base>/debug/rf` and `<base>/debug/i2c` (not retained). There is no separate setting.
- **I2C sniffer.** The reply received during a sniffer pause is logged as `rx[..]` (#403).
- **Virtual remotes.** The bi-directional models (RFT-N, RFT AUTO-N, RFT CO2, RFT Spider and RFT-RV) are no longer offered, they don't work as a virtual remote (#405).
- **Device info.** `/api/v2/deviceinfo` now has `heap_free`, `heap_min_free` and `heap_max_block`.

### Memory

- **More free memory.** On a CVE Silent test device, free memory went from about 75 KB to 100 KB and the lowest point after boot from about 24 KB to 63 KB. The TLS buffers, the MQTT buffer and tables that never change take less RAM, tasks that are done give their memory back, and REST replies and MQTT sends no longer hold extra copies.
- **Free memory shown correctly.** The free memory shown and the low memory checks counted about 60 KB that can't be used. The number you see is lower for the same device, that is expected.
- **Known issue.** Several browser windows loading the add-on at the same moment take a lot of memory for a short time. With four windows the lowest point dropped to about 10 KB in a test, without problems. Close the windows you don't use.

### Bugfixes

- **Crash on low memory.** The web server aborted when memory ran out, for example during the firmware check. It now answers 503 and keeps running. The firmware check waits until after boot or a settings save, and is postponed when memory is low.
- **Firmware check.** Fixed a use-after-free of the TLS client at the end of the check, which could crash the device.
- **Remotes gone after a reboot.** The remotes file also stored live values and could grow past its limit of 4000 bytes, after which all remotes were missing at the next boot. The file now only holds the configuration and the limit is higher. A file that was too large loads again with this version.
- **Manual firmware upload.** With RF status requests on, or in RF standalone mode, the reboot after an upload hung for about a minute until the watchdog reset the device. A slow upload failed completely.
- **RF CO2 join.** The joined remote is now also selected as the RF CO2 remote. In another slot than 0, the slider and keepalive did not work until you selected it yourself.
- **Logging.** Logging stopped for good after a log queue overflow.
- **WiFi settings.** Saving failed with "Unable to write config file". With this fixed, a WPA3-only network works (#303).
- **RF status.** A 31D9 or 31DA frame from a neighbouring unit could be filed under your own device (#406).
- **Boot.** The `GPIO isr service already installed` error at boot is gone, and the I2C sniffer task no longer starts twice.

### Going back to an older firmware

A setting above 12 remotes is safe on older firmware, it then uses 12 slots. Remotes above slot 12 are not available there, and are lost if that firmware saves the remotes file.

### Firmware binary

https://github.com/arjenhiemstra/ithowifi/releases/download/Version-3.3.0-beta8/nrgitho-v3.3.0-beta8.bin
