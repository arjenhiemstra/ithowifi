# Version 3.1.4-beta4

Iterative beta on top of 3.1.4-beta3. Headline change is the structural fix for CVE ECO 2 SP fan control after a cold power cycle — the regression behind issues [#346](https://github.com/arjenhiemstra/ithowifi/issues/346), [#364](https://github.com/arjenhiemstra/ithowifi/issues/364) and [#365](https://github.com/arjenhiemstra/ithowifi/issues/365).

## Bug fix — CVE ECO 2 SP fan control after a cold power cycle

Since 2.9.0-beta13 the boot-time `sendI2CPWMinit()` handshake had been silently skipped on CVE hardware rev "2" because a pin-naming refactor caused the boot-state check (`ithoInitCheck()`) to read the add-on's *output* line back into itself. The line is always LOW (we drove it LOW ourselves in `hardwareInit`), so the gate never opened, and the PWM-mode wake-up frame was never sent.

For modern CVE / HRU units the missing frame was a no-op — they accept PWM2I2C frames without it — so they kept working. But the first-generation CVE ECO 2 SP (`type:0x04 fw:0x01`) still requires the handshake to enable PWM-accept mode, and on every cold boot it would silently drop every PWM frame the add-on sent. Symptom: fan speed reads were fine, slider had no effect.

The boot init state machine now no longer reads any physical pin to decide what to do — `sendI2CPWMinit()` is issued from `initI2cFunctions()` the moment `QueryDevicetype` confirms a CVE unit responded on I2C. This is the strictly stronger and reliable readiness signal that the GPIO read was meant to be a proxy for.

Affected CVE ECO 2 SP users who'd reverted to 2.8.1 should be able to update to this build and have the slider work after both OTA upgrade *and* full power cycle.

## Internal change — status LED behaviour

The boot-time LED indication that the CC1101 module was detected is now a non-blocking 30-second blink instead of a brief blocking on/off pulse. Visible only on units with a CC1101 fitted; otherwise the LED behaves identically to beta3. (Internal testing helper — feel free to ignore unless you're staring at the PCB.)

## Investigation diagnostics

The `I2C: ithoInitCheck status_pin=…` log line from beta3 is gone (the function it lived in has been deleted). The replacement is one log line on every successful boot init: `I2C: sendI2CPWMinit triggered by initI2cFunctions (boot)`. The settings-save trigger keeps its own log line (`triggered by settings save`).

## Upgrade

Users on 2.8.1 who reverted because of the CVE ECO 2 SP regression can pick this beta up via the Update page (beta channel) or directly from the release asset.
