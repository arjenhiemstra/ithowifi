## Version 3.2.0-beta2

Single-fix beta — flushes a long-standing HA Discovery template error for HRU users.

### Bug fixes

- **Fan entity no longer spams `'int object' has no attribute 'Requested fanspeed (%)'` in Home Assistant.** The fan's `percentage_state_topic` is `<base>/state`, which publishes a plain integer (the current ventilation percentage), but the HRU 350 / HRU-eco / HRU 250-300 / CVE-without-PWM2I2C branch of the HA Discovery builder was emitting `{{ value_json['Requested fanspeed (%)'] | int }}` — a JSON-object lookup on what is just a number. HA logged a template error on every state update. Template fixed to `{{ value | int }}`, matching what the CVE-with-PWM2I2C path already did. Pre-existing since the original HA Auto Discovery feature.
