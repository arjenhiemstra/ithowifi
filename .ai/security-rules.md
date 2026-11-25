# Security Rules

Grounded in the auth mechanisms actually implemented — no invented threat model beyond this.

## What exists today

- **REST v1** (`main/api/WebAPIv1.cpp:10-33`): optional query-param `username`/`password` check against `systemConfig.sys_password`, gated by `systemConfig.syssec_api`. Plaintext string compare, no hashing, no HTTPS enforced. Legacy — do not extend this pattern for new endpoints.
- **`SecureWebCommLite`** (`main/managers/SecureWebCommLite.h:13-22`): session-key AES-256-CTR encryption for credentials over plain HTTP/WebSocket. Explicitly documented in its own header as *not* a substitute for HTTPS — it protects against casual traffic inspection and simple replay, not a real threat model. Session key exchanged in plaintext; timeout 3600s.
- Config stores plaintext secrets in `SystemConfig`/`WifiConfig` (`sys_password[33]`, `mqtt_password[33]`) in LittleFS — this is device-local flash, not transmitted at rest.

## Rules for new code

1. Do not add new plaintext-credential-in-query-string endpoints — that pattern (`WebAPIv1`) is legacy, not a template to copy.
2. If an endpoint needs auth, prefer whatever `SecureWebCommLite` already provides over inventing a new scheme.
3. Never log `sys_password`, `mqtt_password`, or WiFi credentials — check `LogConfig`-guarded log statements before adding new ones near config code.
4. This firmware runs on a LAN-facing device with no enforced HTTPS — do not claim or imply transport security guarantees in new docs/UI copy that the code doesn't provide.
5. Vendored TLS/crypto code (`mbedtls`, `software/lib/*`) is out of scope to modify — see [.ai/coding-standards.md](coding-standards.md).

## Related

[.ai/coding-standards.md](coding-standards.md) · [.cursor/rules/security.mdc](../.cursor/rules/security.mdc)
