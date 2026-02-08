#pragma once

#include <mbedtls/aes.h>
#include <mbedtls/entropy.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/base64.h>
#include <mbedtls/platform_util.h>
#include <time.h>

#define SESSION_KEY_SIZE 32  // AES-256
#define NONCE_SIZE 12        // CTR nonce

/**
 * SecureWebCommLite: Session-key based AES-256-CTR encryption
 *
 * This provides credential encryption that works over plain HTTP.
 * The session key is exchanged in plaintext (HTTP limitation), but provides:
 * - Protection against credentials in logs/debug output
 * - Protection against casual WebSocket traffic inspection
 * - Protection against simple replay attacks (random nonce per message)
 *
 * For stronger security, use HTTPS.
 */
class SecureWebCommLite {
private:
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;

    // Session key (randomly generated)
    uint8_t session_key[SESSION_KEY_SIZE];
    bool session_established;
    time_t session_start_time;

    // Session timeout (regenerate key periodically)
    static const uint32_t SESSION_TIMEOUT_SEC = 3600;  // 1 hour

    // Generate new random session key
    bool generate_session_key();

    // Decrypt using AES-256-CTR
    bool decrypt_aes_ctr(const uint8_t *ciphertext, size_t ciphertext_len,
                         const uint8_t *nonce,
                         uint8_t *plaintext, size_t *plaintext_len);

public:
    SecureWebCommLite();
    ~SecureWebCommLite();

    // Initialize RNG (fast: <100ms)
    bool init();

    // Check if session needs new key
    bool needs_new_session();

    // Decrypt a single base64-encoded field, returns true on success
    bool decryptField(const char* ct_b64, const char* nonce_b64, char* out, size_t maxLen);

    // Send session key response for WebSocket
    void send_session_key_response();
};

extern SecureWebCommLite secureWebCommLite;
