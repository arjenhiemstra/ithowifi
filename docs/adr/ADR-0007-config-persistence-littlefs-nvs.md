# ADR-0007: Config Persistence — LittleFS + NVS Backup

Status: Accepted

## Context

Configuration (WiFi credentials, MQTT/system settings, RF remote registrations, logging/HA discovery settings) must survive reboots and OTA updates — and, critically, a **flash repartition**, which rewrites the partition table and wipes the LittleFS filesystem. Evidence: `main/config/Config.h` declares JSON-file-based `load*Config`/`save*Config` functions for `SystemConfig`, `WifiConfig`, `LogConfig`, `HADiscConfig` and `IthoRemote`. Every config type's load path takes an `nvs_backup_key` (`loadConfigFile(..., nvs_backup_key, ...)` for system/wifi/log/hadisc, `loadFileRemotes(..., nvs_backup_key, ...)` for remotes/vremotes — `Config.cpp`), giving each config an NVS (ESP-IDF non-volatile key/value storage) restore path.

## Decision

Use LittleFS JSON files (via ArduinoJson `JsonDocument`) as the primary config store for all config structs. Before a flash repartition, snapshot **all** config types to NVS: `backupAllConfigs()` (`esp32_repartition.cpp`) sets the `use*confb` flags and calls `save*Config("nvs")` for wifi/system/log/remotes/vremotes. On the next boot each config's load path checks its `use*confb` flag and, if set, restores from NVS and clears the flag. NVS is therefore a repartition-survival snapshot, not a continuously-synced second copy.

## Consequences

**Positive**: JSON files are human-readable/debuggable (visible via the web UI file browser, `handlers/FileHandlers.cpp`) and easy to extend with new fields; the NVS snapshot lets config survive the flash repartition that wipes LittleFS, so repartitioning doesn't force the user to reconfigure.

**Negative**: two persistence mechanisms; the NVS copy is a point-in-time snapshot taken at repartition, not kept in sync with ongoing LittleFS writes — it only reflects config as of the last `backupAllConfigs()` call.

## Alternatives Considered

- **NVS for all config, all the time**: rejected — NVS key/value storage doesn't suit large structured JSON documents as well as filesystem-JSON for the normal read/write path; NVS is used only as the pre-repartition snapshot.
- **LittleFS-only, no NVS snapshot**: rejected — a flash repartition would then wipe all config with no recovery path; the `backupAllConfigs()`/`nvs_backup_key` snapshot exists specifically to bridge the repartition.

## Related Components

`main/config/Config.h`, `main/config/Config.cpp`, `main/esp32_repartition.cpp`, `main/config/SystemConfig.h`, `main/config/WifiConfig.h`, `main/config/IthoRemote.h`, `main/handlers/FileHandlers.cpp`

## Related Issues

None tracked.

## AI Notes

New config fields go on the existing structs + JSON load/save functions, following the established `load*Config`/`save*Config` naming (see [.ai/coding-standards.md](../../.ai/coding-standards.md)). A new config category should also be added to the repartition snapshot: give its load path an `nvs_backup_key` and add a `save*Config("nvs")` line to `backupAllConfigs()`, so it too survives a repartition.
