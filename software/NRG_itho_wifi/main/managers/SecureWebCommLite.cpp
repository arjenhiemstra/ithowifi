#include "SecureWebCommLite.h"
#include "sys_log.h"
#include "notifyClients.h"

SecureWebCommLite secureWebCommLite;

SecureWebCommLite::SecureWebCommLite()
    : session_established(false), session_start_time(0)
{
    mbedtls_entropy_init(&entropy);
    mbedtls_ctr_drbg_init(&ctr_drbg);
    memset(session_key, 0, sizeof(session_key));
}

SecureWebCommLite::~SecureWebCommLite() {
    mbedtls_entropy_free(&entropy);
    mbedtls_ctr_drbg_free(&ctr_drbg);
    // Zero session key from memory
    mbedtls_platform_zeroize(session_key, sizeof(session_key));
}

bool SecureWebCommLite::init() {
    I_LOG("SWL: Initializing secure communication");

    uint32_t start_time = millis();

    // Seed RNG
    const char *pers = "itho_aes";
    int ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func,
                                     &entropy,
                                     (const unsigned char *)pers,
                                     strlen(pers));
    if (ret != 0) {
        E_LOG("SWL: RNG seed failed: -0x%04x", -ret);
        return false;
    }

    // Generate initial session key
    if (!generate_session_key()) {
        E_LOG("SWL: Session key generation failed");
        return false;
    }

    uint32_t elapsed = millis() - start_time;
    I_LOG("SWL: Init complete in %ums", elapsed);

    return true;
}

bool SecureWebCommLite::generate_session_key() {
    D_LOG("SWL: Generating session key...");

    // Generate 32 random bytes for AES-256 key
    int ret = mbedtls_ctr_drbg_random(&ctr_drbg, session_key, SESSION_KEY_SIZE);
    if (ret != 0) {
        E_LOG("SWL: Random generation failed: -0x%04x", -ret);
        return false;
    }

    session_established = true;
    session_start_time = time(nullptr);

    D_LOG("SWL: Session key generated");
    return true;
}

bool SecureWebCommLite::needs_new_session() {
    // Check session timeout
    time_t now = time(nullptr);
    if (now - session_start_time > SESSION_TIMEOUT_SEC) {
        I_LOG("SWL: Session expired, generating new key");
        generate_session_key();
        return true;
    }

    return false;
}

bool SecureWebCommLite::decrypt_aes_ctr(const uint8_t *ciphertext,
                                        size_t ciphertext_len,
                                        const uint8_t *nonce,
                                        uint8_t *plaintext,
                                        size_t *plaintext_len) {
    if (!session_established) {
        E_LOG("SWL: No session established");
        return false;
    }

    // Setup AES context
    mbedtls_aes_context aes;
    mbedtls_aes_init(&aes);

    int ret = mbedtls_aes_setkey_enc(&aes, session_key, SESSION_KEY_SIZE * 8);
    if (ret != 0) {
        E_LOG("SWL: AES setkey failed: -0x%04x", -ret);
        mbedtls_aes_free(&aes);
        return false;
    }

    // Prepare counter block: 12-byte nonce + 4-byte counter (starting at 0)
    uint8_t nonce_counter[16];
    memcpy(nonce_counter, nonce, NONCE_SIZE);
    memset(nonce_counter + NONCE_SIZE, 0, 4);  // Counter starts at 0

    // Stream block for CTR mode
    uint8_t stream_block[16];
    size_t nc_off = 0;

    // Decrypt (CTR mode: encrypt = decrypt)
    ret = mbedtls_aes_crypt_ctr(&aes, ciphertext_len,
                                 &nc_off, nonce_counter, stream_block,
                                 ciphertext, plaintext);

    mbedtls_aes_free(&aes);

    if (ret != 0) {
        E_LOG("SWL: Decryption failed: -0x%04x", -ret);
        return false;
    }

    *plaintext_len = ciphertext_len;
    D_LOG("SWL: Decrypted %zu bytes", ciphertext_len);
    return true;
}

bool SecureWebCommLite::decryptField(const char* ct_b64, const char* nonce_b64,
                                     char* out, size_t maxLen) {
    if (!ct_b64 || !nonce_b64 || strlen(ct_b64) == 0 || strlen(nonce_b64) == 0) {
        E_LOG("SWL: Missing encryption parameters");
        return false;
    }

    // Base64 decode
    uint8_t ciphertext[256];
    uint8_t nonce[NONCE_SIZE];
    size_t ct_len, nonce_len;

    int ret = mbedtls_base64_decode(ciphertext, sizeof(ciphertext), &ct_len,
                                     (const uint8_t*)ct_b64, strlen(ct_b64));
    if (ret != 0) {
        E_LOG("SWL: Ciphertext decode failed");
        return false;
    }

    ret = mbedtls_base64_decode(nonce, sizeof(nonce), &nonce_len,
                                (const uint8_t*)nonce_b64, strlen(nonce_b64));
    if (ret != 0 || nonce_len != NONCE_SIZE) {
        E_LOG("SWL: Nonce decode failed");
        return false;
    }

    // Decrypt
    uint8_t plaintext[256];
    size_t plaintext_len;

    if (!decrypt_aes_ctr(ciphertext, ct_len, nonce, plaintext, &plaintext_len)) {
        E_LOG("SWL: Decryption failed");
        return false;
    }

    plaintext[plaintext_len] = '\0';
    strlcpy(out, (char*)plaintext, maxLen);

    // Zero plaintext from memory
    mbedtls_platform_zeroize(plaintext, sizeof(plaintext));

    return true;
}

void SecureWebCommLite::send_session_key_response() {
    // Check if we need a new session key
    needs_new_session();

    // Base64 encode session key for transmission
    char encoded[64];
    size_t encoded_len;
    mbedtls_base64_encode((uint8_t*)encoded, sizeof(encoded), &encoded_len,
                          session_key, SESSION_KEY_SIZE);
    encoded[encoded_len] = '\0';

    // Send via WebSocket
    JsonDocument response;
    response["session_key"] = encoded;
    response["session_timeout"] = SESSION_TIMEOUT_SEC;

    notifyClients(response.as<JsonObject>());

    D_LOG("SWL: Sent session key to browser");
}
