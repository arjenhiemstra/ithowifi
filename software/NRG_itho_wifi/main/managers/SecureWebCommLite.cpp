#include "SecureWebCommLite.h"
#include "config/SystemConfig.h"
#include "config/WifiConfig.h"
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
    I_LOG("SECURE_WEB_LITE: Initializing secure communication");

    uint32_t start_time = millis();

    // Seed RNG
    const char *pers = "itho_aes";
    int ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func,
                                     &entropy,
                                     (const unsigned char *)pers,
                                     strlen(pers));
    if (ret != 0) {
        E_LOG("SECURE_WEB_LITE: RNG seed failed: -0x%04x", -ret);
        return false;
    }

    // Generate initial session key
    if (!generate_session_key()) {
        E_LOG("SECURE_WEB_LITE: Session key generation failed");
        return false;
    }

    uint32_t elapsed = millis() - start_time;
    I_LOG("SECURE_WEB_LITE: Init complete in %ums", elapsed);

    return true;
}

bool SecureWebCommLite::generate_session_key() {
    D_LOG("SECURE_WEB_LITE: Generating session key...");

    // Generate 32 random bytes for AES-256 key
    int ret = mbedtls_ctr_drbg_random(&ctr_drbg, session_key, SESSION_KEY_SIZE);
    if (ret != 0) {
        E_LOG("SECURE_WEB_LITE: Random generation failed: -0x%04x", -ret);
        return false;
    }

    session_established = true;
    session_start_time = time(nullptr);

    D_LOG("SECURE_WEB_LITE: Session key generated");
    return true;
}

bool SecureWebCommLite::needs_new_session() {
    // Check session timeout
    time_t now = time(nullptr);
    if (now - session_start_time > SESSION_TIMEOUT_SEC) {
        I_LOG("SECURE_WEB_LITE: Session expired, generating new key");
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
        E_LOG("SECURE_WEB_LITE: No session established");
        return false;
    }

    // Setup AES context
    mbedtls_aes_context aes;
    mbedtls_aes_init(&aes);

    int ret = mbedtls_aes_setkey_enc(&aes, session_key, SESSION_KEY_SIZE * 8);
    if (ret != 0) {
        E_LOG("SECURE_WEB_LITE: AES setkey failed: -0x%04x", -ret);
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
        E_LOG("SECURE_WEB_LITE: Decryption failed: -0x%04x", -ret);
        return false;
    }

    *plaintext_len = ciphertext_len;
    D_LOG("SECURE_WEB_LITE: Decrypted %zu bytes", ciphertext_len);
    return true;
}

bool SecureWebCommLite::handle_encrypted_credential(JsonObject obj) {
    if (!obj["encrypted_credential"].is<bool>()) {
        return false;
    }

    const char* type = obj["type"] | "";
    const char* username = obj["username"] | "";

    // Get encrypted password data
    const char* ciphertext_b64 = obj["ciphertext"] | "";
    const char* nonce_b64 = obj["nonce"] | "";

    if (strlen(ciphertext_b64) == 0 || strlen(nonce_b64) == 0) {
        E_LOG("SECURE_WEB_LITE: Missing encryption parameters");
        return false;
    }

    // Base64 decode
    uint8_t ciphertext[256];
    uint8_t nonce[NONCE_SIZE];
    size_t ct_len, nonce_len;

    int ret = mbedtls_base64_decode(ciphertext, sizeof(ciphertext), &ct_len,
                                     (const uint8_t*)ciphertext_b64,
                                     strlen(ciphertext_b64));
    if (ret != 0) {
        E_LOG("SECURE_WEB_LITE: Ciphertext decode failed");
        return false;
    }

    ret = mbedtls_base64_decode(nonce, sizeof(nonce), &nonce_len,
                                (const uint8_t*)nonce_b64, strlen(nonce_b64));
    if (ret != 0 || nonce_len != NONCE_SIZE) {
        E_LOG("SECURE_WEB_LITE: Nonce decode failed");
        return false;
    }

    // Decrypt password
    uint8_t plaintext[256];
    size_t plaintext_len;

    if (!decrypt_aes_ctr(ciphertext, ct_len, nonce,
                         plaintext, &plaintext_len)) {
        E_LOG("SECURE_WEB_LITE: Decryption failed for %s credential", type);
        return false;
    }

    plaintext[plaintext_len] = '\0';

    // Update credentials
    bool updated = false;

    if (strcmp(type, "sys") == 0) {
        strlcpy(systemConfig.sys_username, username,
                sizeof(systemConfig.sys_username));
        strlcpy(systemConfig.sys_password, (char*)plaintext,
                sizeof(systemConfig.sys_password));

        updated = true;
        I_LOG("SECURE_WEB_LITE: Updated system credentials");

    } else if (strcmp(type, "mqtt") == 0) {
        strlcpy(systemConfig.mqtt_username, username,
                sizeof(systemConfig.mqtt_username));
        strlcpy(systemConfig.mqtt_password, (char*)plaintext,
                sizeof(systemConfig.mqtt_password));

        updated = true;
        I_LOG("SECURE_WEB_LITE: Updated MQTT credentials");

    } else if (strcmp(type, "wifi") == 0) {
        strlcpy(wifiConfig.ssid, username, sizeof(wifiConfig.ssid));
        strlcpy(wifiConfig.passwd, (char*)plaintext, sizeof(wifiConfig.passwd));

        updated = true;
        I_LOG("SECURE_WEB_LITE: Updated WiFi credentials");

    } else if (strcmp(type, "wifiap") == 0) {
        strlcpy(wifiConfig.appasswd, (char*)plaintext, sizeof(wifiConfig.appasswd));

        updated = true;
        I_LOG("SECURE_WEB_LITE: Updated WiFi AP password");
    }

    // Zero plaintext from memory
    mbedtls_platform_zeroize(plaintext, sizeof(plaintext));

    return updated;
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

    D_LOG("SECURE_WEB_LITE: Sent session key to browser");
}
