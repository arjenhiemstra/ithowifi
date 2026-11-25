## Version 3.3.0-beta11

Eleventh beta of the 3.3 line. RF task hardening after a reported RF dropout, a second variant of the old Itho RF protocol and a fix for the remote buttons in the web interface.

### Bugfixes

- **RF stopped after "CC1101 Task timed out!".** A user on beta10 saw that line once and then no RF reception at all while the rest of the add-on kept running. The RF task was not under the task watchdog (lost in the 2.5.0 refactor), so a hang in it never caused a reset or a coredump, and the waits on the radio chip had no time limit. The RF task is now watched again, every wait on the chip has a timeout, a read loop gives up after 32 tries, and the task checks once a second that the chip is receiving and restarts the receive when it is not. The RFS line gains `rxrestart` and `chiptimeout` counters; a warning is logged at most once per 10 minutes.
- **Remote buttons ran twice.** Every button on the Virtual remotes and RF devices pages also executed the command as a plain speed command: a PWM speed set with the API-to-vremote mapping off, or a duplicate command to virtual remote 0 with it on. Introduced in beta10 with the join result poll.

### Features

- **Old Itho RF protocol, variant 2.** Some old-protocol remotes send every command with the lowest bit flipped (low EFFC, medium EFF9, high EFF5, timer1 DDEB), captured from a user's remote. New remote type "RF (old protocol, variant 2)" sends that variant for units that ignore the normal one. Try "RF (old protocol)" first; if the unit does not react after a join, use variant 2. The RF log labels codes of both variants, so EFF5 shows as full|high(v2) and EFFC as low(v2).

### API

- The API page documents the `vent_demand`, `vent_demand_zone` and `vent_demand_flags` fields of `remotesinfo` (#413).

### Firmware binary

https://github.com/arjenhiemstra/ithowifi/releases/download/Version-3.3.0-beta11/nrgitho-v3.3.0-beta11.bin
