# Version 3.1.4-beta5

Iterative beta on top of 3.1.4-beta4. Adds add-on-tracked timer visibility for PWM2I2C timer / cook commands.

## Feature — expose add-on timer remaining on `/api/v2/speed`

For CVE / HRU200 units controlled via PWM2I2C, the `timer1` / `timer2` / `timer3` and `cook30` / `cook60` web-UI buttons drive the fan to a fixed speed for N minutes via the add-on's internal `IthoQueue`. The Itho unit itself only sees a PWM speed write, so the protocol-level `FanInfo` / `Selection` / `RemainingTime` fields stay at their pre-timer values — there's no way for a consumer (HA integration, MQTT subscriber, dashboard) to tell that a timer is running.

The `/api/v2/speed` endpoint now also returns the head-of-queue timer state from `ithoQueue`:

```json
{
    "status": "success",
    "data": {
        "currentspeed": 220,
        "timer_remaining_ms": 581600,
        "timer_speed": 220,
        "timestamp": 1780600058
    }
}
```

`timer_remaining_ms` is the milliseconds left on the active (head-of-queue) timer (0 when no timer is queued). `timer_speed` is the PWM speed value that was queued for that timer (`-1` when none).

The HA integration v0.5.7 picks this up automatically and adds an "Add-on timer remaining" diagnostic sensor in minutes. Older integration versions ignore the new fields safely. Issue [#368](https://github.com/arjenhiemstra/ithowifi/issues/368).

This is purely additive — no existing field renamed or removed, no behaviour change on the I2C side.
