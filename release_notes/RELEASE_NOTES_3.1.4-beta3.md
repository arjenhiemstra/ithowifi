# Version 3.1.4-beta3

Iterative beta on top of 3.1.4-beta2. Two functional changes, plus diagnostics that will help us close out [#365](https://github.com/arjenhiemstra/ithowifi/issues/365) (CVE ECO 2 SP fan control after power cycle).

## Bug fix

- **RF CO2 demand dispatch now respects the unit's current mode.** The 3.1.4-beta2 fix removed the unconditional `auto` precursor before the 31E0 demand frame. That fixed "drag slider down while in auto" but broke "drag slider while not in auto", because the unit only accepts a 31E0 frame when in auto mode. This release reads the unit's `FanInfo` (from the I2C 31DA path when `itho_31da` is enabled, or from a tracked RF 31DA source in RF-standalone mode) and only precedes the demand with an `auto` command when the unit isn't already in auto. If `FanInfo` is unknown (no 31DA polling configured), the firmware errs on the side of sending `auto + demand`, matching pre-3.1.4-beta2 behaviour for that case.

  Affects:
  - Web UI index-page demand slider (RF CO2 mode)
  - `POST /api/v2/command` with `percentage` or `fandemand` (the path used by the HA integration)
  - MQTT `cmd` topic with `percentage` or `fandemand`

  For RF-standalone setups, configure an **RF status source** (or enable I2C 31DA polling) so the firmware can see the unit's current mode and pick the optimised path. Without it the firmware always sends `auto + demand`, which works but re-introduces the boost-mode side-effect when the unit was already in auto.

## Diagnostics (no behaviour change)

Investigating the CVE ECO 2 SP "fan stops responding to slider after a power cycle" reports in [#346](https://github.com/arjenhiemstra/ithowifi/issues/346) / [#364](https://github.com/arjenhiemstra/ithowifi/issues/364) / [#365](https://github.com/arjenhiemstra/ithowifi/issues/365) traced to a regression in 2.9.0-beta13 where the boot-time `sendI2CPWMinit()` handshake (the I2C frame that asks the unit to enable PWM-over-I2C acceptance) silently stopped firing on cold boot. The 2.9.0-beta13 commit that simplified pin definitions repurposed `status_pin` on CVE hardware rev "2" from an INPUT (reading the unit's readiness signal) to an OUTPUT (signalling our readiness to the unit), without updating the `digitalRead(status_pin)` in `ithoInitCheck()`. So on cold boot the firmware reads its own output line — which it just drove LOW — and never opens the gate that calls `sendI2CPWMinit()`.

Only `type:0x04 fw:0x01` units (the very first CVE I2C-protocol generation, sold as CVE ECO 2 SP) actually require the handshake; every other CVE / HRU variant accepts PWM2I2C frames directly. That's why the bug has been silent on almost every unit.

This release adds three log lines to confirm the diagnosis on affected hardware before shipping the fix:

- `I2C: ithoInitCheck status_pin=LOW/HIGH, pwm2i2c=N` — surfaces what `ithoInitCheck()` actually reads on cold boot.
- `I2C: sendI2CPWMinit triggered by ithoInitCheck (boot)` — fires only when the cold-boot path successfully calls `sendI2CPWMinit()`.
- `I2C: sendI2CPWMinit triggered by settings save` — fires when the websocket settings-save handler calls it (the path that, when triggered by toggling the debug switch, currently "fixes" the problem until the next power cycle on affected units).

If you have a CVE ECO 2 SP whose slider stops responding after a power cycle, please install this beta and share the System Log around the boot + the moment you toggle the I2C debug switch. The expected pattern is `status_pin=LOW`, no `triggered by ithoInitCheck (boot)` line, then `triggered by settings save` the moment you save any setting.

## Carries forward from 3.1.4-beta1 and beta2

All the memory-stability and I2C-status-format-flake fixes from beta1, and the index-page / REST / MQTT demand-slider fix from beta2.
