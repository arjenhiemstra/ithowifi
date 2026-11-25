## Version 3.3.0-beta4

Fourth beta of the 3.3 line. Mostly fixes for beta3 feedback and open issues.

**Note for Home Assistant users:** the fan, firmware update and per remote entities now get their ID from the hostname (#387). Home Assistant recreates these entities once after the update; changes you made to them in Home Assistant (name, area, entity ID) have to be redone.

### Features

- **I2C debug and sniffer as toggles.** The debug page shows a toggle and the current status for both, like the JavaScript console debug. The sniffer switches at once, no reboot needed. The log shows a warning at boot while either is on.
- **Zone in the MQTT rfstatus message (#383).** The 31DA message has a `zone` field, so on a multi-zone unit (DuoZone) you can tell which zone the values belong to.

### Bugfixes

- **Filter, fault and speed status over RF in RF CO2 mode.** Since 3.3.0-beta1 an RF only setup with an RFT CO2 remote no longer requested the fan status, so these values stayed unknown. The status request runs again, also when "Periodic 31DA/31D9 status request" under RF CO2 is off.
- **Settings could not be saved after a broken config file.** A reset during a save could leave a config file empty; after that every change was lost at the next reboot. Config files are now written via a temporary file, and an unreadable file is moved to `<file>.bad` and replaced by the defaults.
- **Spider discovery target 21.5 to 24 °C** instead of 23 to 30 °C. Spiders on an AutoTemp go up to 24 °C.
- **HA entity IDs clashed between two add-ons of the same type (#387).** The fan, firmware update and per remote entities used the device name; they now use the hostname, like the other entities.
- **HA fan presets for CVE and HRU 200 with PWM2I2C (#384).** The fan always got the generic list with Auto/AutoNight and the lowest speed showed as Auto. It now uses the presets of virtual remote 0 when API commands go to that remote, otherwise Low, Medium, High and the timers.
- **HRU-eco fan in HA (#388).** The preset template read the speed from a fixed position that does not exist on most HRU-eco firmware versions and failed on every status update. The speed is now looked up by its label.
- **HA template error on an empty last command (#392).**
- **REST settings return HTTP 503 when the I2C command fails (#390)**, so clients know they can retry.

### Firmware binary

https://github.com/arjenhiemstra/ithowifi/releases/download/Version-3.3.0-beta4/nrgitho-v3.3.0-beta4.bin
