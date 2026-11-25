## Version 3.3.0-beta2

Second beta of the 3.3 line. Adds Orcon RF support and a way to silence the firmware-update check on offline setups, plus a batch of RF fixes and a crash fix.

### Features

- **Orcon 15RF (MVS) RF send support.** New **Orcon 15RF** remote type — control an Orcon MVS-15 unit over RF by emulating a 15RF remote.
- **Orcon CO2 RF demand control.** New **Orcon CO2** remote type; the RF CO2 controls (system settings + index page) target the specifically configured remote, and demand is sent as a single command.
- **Automatic firmware update check can be turned off.** New toggle under **System settings → System security**. If the add-on has no internet access (or its outbound connections are blocked), turn it off to stop the periodic GitHub version check and the repeated failed-connection log entries it produces.

### Bugfixes

- **RF remote ID no longer lost when changing a remote's function (#376).** Toggling a remote to Receive and back to Send used to overwrite its ID, breaking joined remotes and making multiple Send remotes collide. The ID is now preserved across the toggle, and a new Send remote gets a unique, unused ID.
- **Fixed a status-read crash** caused by an uninitialized device-status field (could reboot the add-on when reading Itho status).
- **Fixed an RF status data race** — the RF status measurement vectors are now protected by a dedicated mutex.
- **More reliable RF bind-confirm**, with a cloned-remote broadcast fallback.

### Performance

- **Faster RF I/O** — the CC1101 SPI bus now runs at 4 MHz (was 1 MHz) with block FIFO transfers, cutting CPU time on the RF path.
- **Smaller web UI** — the interface JavaScript is minified at build time (~63 KB → ~53 KB compressed).

### Under the hood

- Internal refactor of the boot/startup sequence.
- Accessibility fixes for settings-page form controls (label associations, form-field ids).
