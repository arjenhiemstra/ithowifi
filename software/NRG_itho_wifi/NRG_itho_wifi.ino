


/*
 * 
 * Info to compile the firmware yourself:
 * Build is done using Arduino IDE, used libs are downloaded when cloning from git.
 * 
 * For CVE rev 2 (module for itho fans):
 * Select 'ESP32 Dev Module' as board
 * Choose partition scheme: Minimal SPIFFS (1.9 APP with OTA)
 * goto file hardware.h and uncomment CVE and comment NON_CVE define
 * 
 * For non CVE rev 1 (module with RJ45 connector):
 * Select 'ESP32 Dev Module' as board
 * Choose partition scheme: Minimal SPIFFS (1.9 APP with OTA)
 * goto file hardware.h and uncomment NON_CVE and comment CVE define
 * 
 */

/*
 Backlog:
 * (todo) make front end buttons dependent on vremote0 type
 * (todo) check load of remotes config file between 2.3.5 and 2.4.0
 * (todo) After timer, go back to fallback or last value
 * (todo) Prevent flash write during interrupt to prevent crashes
 * (todo) add support for 536-0106 remote, https://github.com/arjenhiemstra/ithowifi/issues/78
 * (todo) make status parameters selectable to include in update or not
 * 
 */
