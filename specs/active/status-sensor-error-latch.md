# Bugfix Spec: 2401 status sensor errors latch until reboot

Status: Active
Related issue: #295 (also #61)

## Symptom

After days to weeks, `RelativeHumidity` and/or `Temperature` read "not available" and the derived `temp`, `hum` and `ppmw` stop updating, while the other status fields keep flowing. Only a reboot, or the `statusformat` debug command, brings them back. `Highest CO2 concentration (ppm)` and `Highest RH concentration (%)` behave the same way.

## Root Cause

`sendQueryStatus` (`main/ithodevice/IthoStatus.cpp`, Version-3.2.0 L236-283, develop L243-290) switched a field's type to `is_string` when one poll returned a humidity or temperature outside 1 to 100, or the sensor error code (0x8200 for CO2, 0xEF for highest RH). The per-field update only handles `is_byte`, `is_int` and `is_float`, so from then on the field was never parsed again, until `sendQueryStatusFormat` rebuilt `ithoStatus` at boot or on `statusformat`. A single bad read latched the field.

## Fix

A field keeps its type. `ithoStatusUpdateField` sets the field's `error` from the current value on every poll, `getIthoStatusJSON` shows the error text while it is set, and `ithoHum`/`ithoTemp` only take a value that has no error. The per-field update lives in `main/ithodevice/IthoStatusField.h`, free of Arduino and FreeRTOS, so `test_native_status_field` runs the firmware's own code.

## Acceptance Criteria

- [x] A humidity or temperature field reads its value again on the first valid poll after an out-of-range one
- [x] The CO2 and highest RH fields read their value again on the first poll after the sensor error code
- [x] While a value is out of range or the error code, the field shows "not available" in the status JSON, as before
- [x] Decoding of byte, int, signed and float fields is unchanged (characterization tests)
- [ ] Live hardware: no freeze for several weeks on a CVE-S Optima that latched three times in four weeks on 3.2.0

## Architecture Impact

- Touches: none. The status JSON, MQTT and REST output keep their shape.

## Risks

- `ithoHum`/`ithoTemp` are now guarded by the field's `error` instead of an inline range check, so a bad read must not reach them. The guard is in `sendQueryStatus`, which the native tests do not cover.
- A sensor that keeps failing now alternates between a value and "not available" instead of freezing on its last value.

## Test Requirements

- [x] Native unit test reproducing the bug, now passing (`test_native_status_field`)
- [ ] Live-hardware verification

## Rollback Strategy

Revert the commits; fields latch again as before.

## Related Components

`main/ithodevice/IthoStatusField.h`, `main/ithodevice/IthoStatus.cpp`, `main/ithodevice/IthoDevice.h`, `main/ithodevice/IthoInfoHelpers.cpp`, `main/generic_functions.cpp`
