## Version 3.3.0-beta7

Seventh beta of the 3.3 line. Itho Spider gateway bind out of experimental, Air to Water Heatpump support and a reworked I2C debug page.

### Features

- **Itho Spider: gateway bind no longer experimental.** The add-on now answers the Spider's `W 22C9` (dial change) and `W 22D0` reports. Without these answers the Spider repeated them and its COMMUNICATIE symbol kept blinking. A setpoint change on the Spider itself is kept: the add-on no longer answers the Spider's next poll with the old value, which made the Spider jump back and blink for a few minutes. A dial change also becomes the commanded setpoint, so the web page and HA no longer show the old value as pending.
- **Itho Spider on an AutoTemp: setpoint always on.** The RF setting "Itho Spider on a zone controller (experimental)" is gone. Discovery now always gives a Spider on an AutoTemp the setpoint function. In some setups the Spider overrules a setpoint sent this way; use Bind as gateway there.
- **RF devices page.** The Spider section explains when to use Bind as gateway and when discovery, with Bind as gateway first.
- **Itho Air to Water Heatpump.** Supported as AWHeatpump (device group 4, id 8), with settings and status labels for firmware 1 and 2. The setup treats it like the WPU: no virtual remote, no 31DA/31D9. Tested by a user on firmware 2.
- **I2C debug page.** The low level I2C commands share one result field. New: serial number, article number, heat pump faults, identify, start deaeration and a generic I2C message sender. The device type query now shows group, id, hardware and firmware version and name. The fault history has its own section at the bottom of the page, with fault texts per device and firmware version where known.

### Bugfixes

- **Shifted status values with the I2C sniffer on (#295).** About one in a thousand replies to a status query lost a byte while the sniffer's interrupt ran, which shifted all values after it for that poll. The sniffer interrupt is now off while the add-on receives a reply. Thanks to @polroskam (#403).
- **Device labels.** Settings and status labels are checked and regenerated. Fixed: CVE-Silent fw 9 settings from index 26 were off by one, HRU ECO-fan fw 2 to 8 missed the trailing "not used" settings, AutoTemp fw 3 and 4 had the power and OEM code labels swapped, CVE fw 1 had status labels 0 and 1 swapped. New: HRU 350 fw 5 and DemandFlow fw 5 settings. Status label keys (MQTT, HA) are unchanged.
- **I2C debug.** The CE30 result was never shown.

### Firmware binary

https://github.com/arjenhiemstra/ithowifi/releases/download/Version-3.3.0-beta7/nrgitho-v3.3.0-beta7.bin
