# Version 3.1.4-beta7

Iterative beta on top of 3.1.4-beta6. Stops the device from panicking when MQTT Home Assistant Discovery runs out of memory on devices with many status items.

## Fix — HA Discovery no longer panics on heap exhaustion

`mqttHomeAssistantDiscovery` could hit `std::bad_alloc` inside `generateHADiscoveryJson` on devices with many status items (134 on a WPU). The exception was uncaught, so the runtime called `std::terminate()` → `abort()` → panic → reboot loop. Total free heap could read fine at the point of failure — the heap was just fragmented from the many small `std::string` / `JsonDocument` allocations the JSON build does.

Two changes:

- The pre-flight heap check now guards on the largest contiguous free block (`heap_caps_get_largest_free_block(MALLOC_CAP_8BIT)`) instead of total free. Total free can read fine while contiguous is too small for the allocations the build actually needs.
- The build + publish is wrapped in `try { … } catch (const std::bad_alloc&) { … }`. If fragmentation tips the heap mid-run after the gate passes, the firmware logs `HAD: out of memory during HA Discovery (largest free block now N), skipping` and bails instead of taking the whole device down.
