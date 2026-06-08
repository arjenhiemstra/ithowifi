# Version 3.1.4-beta7

Iterative beta on top of 3.1.4-beta6. Stops the device from crashing when MQTT Home Assistant Discovery runs out of memory on devices with many status items (notably WPU).

## Fix — HA Discovery no longer panics the device on heap exhaustion

A coredump from a WPU user on 3.1.3 showed `mqttHomeAssistantDiscovery` hitting `std::bad_alloc` inside `generateHADiscoveryJson`. The exception was uncaught, so the runtime called `std::terminate()` → `abort()` → panic → reboot loop. Total free heap was still ≥100 KB at the point of failure — the heap was just badly fragmented from the many small `std::string` and `JsonDocument` allocations that the JSON build does for a device with 134 status items.

Two changes:

- The pre-flight heap check now guards on the **largest contiguous free block** (`heap_caps_get_largest_free_block(MALLOC_CAP_8BIT)`) instead of total free heap. Total free can read fine while contiguous is too small to satisfy any of the allocations the build actually needs.
- The build + publish is wrapped in `try { … } catch (const std::bad_alloc&) { … }`. If fragmentation tips the heap mid-run after the gate passes, the firmware logs `HAD: out of memory during HA Discovery (largest free block now N), skipping` and bails — instead of taking the whole device down.

## Note for users still on 3.1.3 with reboot loops

If you're seeing reboot loops on 3.1.3 after MQTT connects, updating to this beta should stop them. You'll get this fix plus the heap-fragmentation fixes that already shipped earlier in the 3.1.4 line — in particular, 3.1.3 was re-cycling the MQTT publish buffer (shrink-back-to-default after every status publish, regrow on the next one) thousands of times per day, which on its own fragments the heap badly enough to make HA Discovery fail eventually. That re-cycle was removed in 3.1.4-beta1.

## Still to come

The discovery itself will still skip on heavily fragmented devices (notably WPU with all 134 status items enabled) — the proper fix is to stream the discovery payload component-by-component so the in-memory document never gets big in the first place. That's the next step for this work.
