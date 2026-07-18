# Release Process

Derived from `platformio.ini`, `release_notes/`, and git history — not invented.

## Version source of truth

`software/NRG_itho_wifi/platformio.ini` → `[env] build_flags = -D VERSION=<x.y.z[-betaN]>`. Bump this to cut a new version.

## Build environments (`platformio.ini`)

| Env | Purpose | Flag |
|---|---|---|
| `dev` | Local development | `-DENABLE_SERIAL` |
| `beta` | Beta firmware build | `-DBETA` |
| `release` | Stable firmware build | `-DSTABLE` |
| `debug` | JTAG/FTDI hardware debugging | `build_type = debug` |
| `native_test` | Host-side unit tests | n/a (no board) |

## Release notes

One file per version in `release_notes/RELEASE_NOTES_<version>.md` (e.g. `RELEASE_NOTES_3.3.0-beta1.md`). Write one when cutting a version; don't fold multiple versions into one file.

## Commit convention for release commits

`release: version <x.y.z>` or `release: build firmware <x.y.z> and add release notes` — match this pattern (see `.ai/development-workflow.md` for general commit prefixes).

## CI

`.github/workflows/ci.yml` runs on push/PR to `develop`/`master`:
1. `native-tests` job: `pio test -e native_test`
2. `build` job: `pio run -e release`, reports firmware RAM/Flash size to the job summary

CI does **not** run the `tests/api`/`tests/mqtt` live-hardware suite (no device available) — run that manually before publishing a release.

## Related

[.ai/testing-strategy.md](testing-strategy.md) · [.ai/development-workflow.md](development-workflow.md)
