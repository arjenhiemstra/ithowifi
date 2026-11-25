## Version 3.3.0-beta5

Fifth beta of the 3.3 line. Itho Spider on an AutoTemp, and the virtual remote join.

### Features

- **Itho Spider on a zone controller (AutoTemp), experimental.** New option on the RF settings page, off by default. With the option on, the Spider wizard binds a Spider on an AutoTemp to its zone, and a new setpoint is sent as the Spider to the AutoTemp, the same way the Spider sends a dial change. Nothing is sent to the Spider itself. Untested on air: please report whether the AutoTemp takes the setpoint and whether the Spider display follows (with an RF log at debug level 3). Switch the option on before running the wizard, or run the wizard again.
- **Join from a selectable virtual remote (#397).** "Send join command: next power on" always used virtual remote 0; you can now choose the virtual remote in the system settings.

### Bugfixes

- **Spider wizard on an AutoTemp.** A Spider on an AutoTemp (zone controller) was not found: it hung on "Listening for its heater" or reported "not a bound Spider thermostat". It is now found and added read only (room temperature and setpoint), unless the experimental option above is on.
- **Join texts (#397).** The system settings and virtual remotes pages explain that the Itho unit only accepts a join shortly after it gets power, so power cycle the unit; a restart of only the add-on uses up the join. The green "Copy ID" countdown now says to press join on the physical remote.

### Firmware binary

https://github.com/arjenhiemstra/ithowifi/releases/download/Version-3.3.0-beta5/nrgitho-v3.3.0-beta5.bin
