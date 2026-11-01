# Bugfix Spec: the I2C sniffer interrupt can cost a query reply a byte

Status: Active
Related issue: #295

## Symptom

With the I2C sniffer on, about one in a thousand replies to the 2401 status query arrives with one 0x00 missing and a 0x00 appended. The reply keeps its length and passes `checksum()`, so every field after the missing byte is decoded shifted for that poll: counters and the highest CO2/RH values publish wrong numbers, and a humidity or temperature reads out of range and shows "not available". With only the I2C safe guard on, a CVE-S Optima showed one such reply in about 55 hours.

## Root Cause

The sniffer's level-3 SDA interrupt (`gpio_sda_handler`, `main/ithodevice/i2c_sniffer.cpp`) busy-waits through every transaction it sees, up to 14.9 ms, on the same core as the I2C slave driver. When it runs through the Itho's reply, `i2cSlaveReceive` (`main/ithodevice/i2c_esp32.cpp`) can lose a byte. A capture with the add-on's own sniffer had the reply whole on the bus in all six bad replies out of 5699. With the interrupt kept off during the add-on's query and reply, and the sniffer on otherwise, there were none in 31 hours, about 11200 replies.

With the safe guard on, the interrupt is off for 7.5 s after each sensor read and on otherwise, and the guard's timer can switch it back on while a query runs.

## Fix

`i2cSlaveReceive` pauses the sniffer interrupt before it installs the slave driver and resumes it after removing the driver. The interrupt is on only while the sniffer is enabled and not paused, so an enable during a pause (the safe guard's timer, the interrupt re-arming itself after a transaction, the debug toggles) takes effect when the pause ends. Hardware that is not sniffer capable is untouched.

## Acceptance Criteria

- [x] Live hardware, sniffer on, with the interrupt off from before the query until after the reply (an earlier experiment on 3.2.0, CVE-S Optima): no bad status reply in 31 hours, and Home Assistant got humidity updates at the same rate as before, so no replies went missing instead
- [ ] Live hardware, sniffer on, with this change: no bad and no failed status replies
- [ ] While on, the sniffer still shows the rest of the bus traffic, and the safe guard still holds the queue around the fan's sensor read

## Architecture Impact

- Touches: none. `i2c_esp32.cpp` calls into the sniffer for the pause.

## Risks

- A pause lasts as long as the slave receive, at least its 50 ms read timeout. The sniffer does not show the add-on's own replies, or anything else on the bus in that time.
- The safe guard holds the I2C queue around the fan's sensor read, but calls that bypass the queue (REST 2410, RF-triggered 31DA/31D9, websocket 4210) can still receive over it. The guard then misses that read and sets no window for the next one.

## Test Requirements

- No native test: the change is interrupt timing on the ESP32, which the native suite does not compile.
- [ ] Live-hardware verification of this change

## Rollback Strategy

Revert the commit; the interrupt runs during slave receives again.

## Related Components

`main/ithodevice/i2c_sniffer.cpp`, `main/ithodevice/i2c_sniffer.h`, `main/ithodevice/i2c_esp32.cpp`
