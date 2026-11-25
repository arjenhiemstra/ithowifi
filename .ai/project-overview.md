# Project Overview

**ithowifi** — ESP32 firmware that bridges Itho Daalderop ventilation/heat-pump units to WiFi. Hobby project (GPLv3), not a commercial engineering codebase — keep changes pragmatic, avoid over-engineering.

## What it does

- Controls fan speed, timers, and settings on Itho central ventilation units
- Two physical connection methods: **I2C** (wired add-on to the unit) and **RF/CC1101** (wireless, standalone mode e.g. HRU 400)
- Exposes control/status via: REST API v2, MQTT (+ Home Assistant MQTT Discovery), WebSocket (web UI), legacy REST v1
- OTA firmware updates from the web UI

## Hardware variants

| Variant | Connection | Example units |
|---|---|---|
| CVE add-on | I2C | CVE ECO 2, CVE ECO RFT, CVE-S ECO/PAK/Optima/CO2, HRU 200 ECO |
| non-CVE add-on | I2C | HRU ECO fan, HRU 150/200/250/300/350, DemandFlow/QualityFlow, WPU 4G/5G, AutoTemp |
| RF standalone (CC1101) | Wireless (31DA/31D9 packets) | HRU 400, most Itho devices with RF |

Device-specific behavior is **not** a class hierarchy — it's a data table (`ithoDevices[]`). See [.ai/common-patterns.md](common-patterns.md).

## Toolchain

- PlatformIO, `platform = espressif32 ~6.12.0`, `framework = arduino, espidf` (dual framework)
- C++20 (`gnu++2a`), `-Wall -Wextra`
- Project root for PlatformIO commands: `software/NRG_itho_wifi/`

## Integrations

- Home Assistant (REST-based and MQTT-based companion integrations, external repos)
- Homey (REST API)
- Domoticz (MQTT bridge topics)

## Full architecture

See [docs/architecture/repository-map.md](../docs/architecture/repository-map.md) for the complete map (folders, task chain, data flow, build pipeline). This file stays a short orientation summary — do not duplicate the map here.
