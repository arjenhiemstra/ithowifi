# Terminology (protocol / code abbreviations)

| Abbreviation | Meaning |
|---|---|
| DG | Device Group — part of an Itho device's identity key in `ithoDeviceType` |
| ID | Device ID within a group — the other half of the device identity key |
| 31DA | RF/status packet type carrying detailed ventilation status (multi-zone capable) |
| 31D9 | RF/status packet type carrying fan-demand style status |
| GDO | CC1101 "General Digital Output" pin — used as the RX interrupt source (`itho_irq_pin`) |
| ISR | Interrupt Service Routine — see `ITHOinterrupt`, `IRAM_ATTR`-marked, must stay non-blocking |
| NVS | Non-Volatile Storage — ESP-IDF key/value flash storage, used here as remote-registration backup |
| LittleFS | Flash filesystem used for primary JSON config storage |
| JSend | Response envelope convention (`status`/`data`/`message`) used by `ApiResponse` |
| MDNS | Multicast DNS — used for local hostname discovery (`MDNSinit()`) |
| WPU | Warmtepomp Unit (heat pump) device family |
| CC1101 | Texas Instruments sub-GHz RF transceiver chip, the RF hardware this project drives |

## Related

[.ai/glossary.md](glossary.md) — domain/hardware terms · [.ai/architecture-summary.md](architecture-summary.md)
