## Version 3.3.0-beta2

Adds a Simulated Device Runtime for development and testing, on top of the 3.3.0-beta1 RF CO2 keep-alive feature.

### Features

- **Simulated Device Runtime.** The add-on can impersonate an Itho device at the I2C transport layer, so the rest of the firmware (status parsing, MQTT, REST, WebSocket, HA discovery) runs completely unchanged against synthetic telemetry. Useful for developing and testing without physical hardware attached.
  - Configurable via `sim_active`, `sim_profile`, `sim_seed` and `sim_scenario` system settings, exposed through the debug page and REST API.
  - Several device profiles and scenarios (normal, boost, fault, humidity spike, CO2 rise) drive deterministic telemetry from a fixed seed, so runs are reproducible.
  - Mutually exclusive with RF standalone mode — enabling RF standalone turns simulation off.
  - Covered by a native test suite (`test/test_native_simulation`) that runs the exact same simulation core used on-device.
