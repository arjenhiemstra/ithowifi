## Version 3.3.0-beta6

Sixth beta of the 3.3 line. Itho Spider gateway bind and RF module detection.

### Features

- **Bind as gateway (Itho Spider), experimental.** New button on the RF devices setup page. It opens a 3 minute window in which the add-on accepts a Spider's heating pairing offer, the same way an Itho gateway does. Put the Spider in pairing mode within those 3 minutes. The Spider then polls the add-on for its setpoint and takes over a new setpoint, also on its display. A new setpoint is also sent right away, so a wired Spider takes it at once and a battery Spider at its next poll or when its display is touched. Meant for Spiders on an AutoTemp, where the zone route of beta5 is overruled by the Spider re-sending its own setpoint. Untested on air: please report whether the bind works, whether the setpoint holds and whether the AutoTemp still follows the Spider (with an RF log at debug level 3). Binding moves the Spider's heating side to the add-on; if it does not work out, pair the Spider to its AutoTemp again.

### Bugfixes

- **Spider dial change undone.** On a Spider bound to the add-on, a setpoint change on the Spider itself was undone at its next poll, because the add-on kept answering with the last setpoint it had sent. The add-on now answers with its own setpoint only while that is being delivered, and a change on the Spider cancels it.
- **RF module not found (#400).** When the CC1101 does not answer at boot, detection is now retried after a reset of the chip, and then at a lower SPI speed. The log shows a warning when one of the retries was needed.

### Firmware binary

https://github.com/arjenhiemstra/ithowifi/releases/download/Version-3.3.0-beta6/nrgitho-v3.3.0-beta6.bin
