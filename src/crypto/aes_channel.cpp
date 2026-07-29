#include "aes_channel.h"

#include <algorithm>
#include <cstring>

#include <esp_system.h>
#include <mbedtls/aes.h>
#include <mbedtls/sha256.h>

namespace crypto {

namespace {

std::vector<uint8_t> pkcs7Pad(const std::vector<uint8_t> &data) {
    size_t padLen = AES_BLOCK_LEN - (data.size() % AES_BLOCK_LEN);
    std::vector<uint8_t> out = data;
    out.insert(out.end(), padLen, static_cast<uint8_t>(padLen));
    return out;
}

// Returns false if the padding is malformed.
bool pkcs7Unpad(std::vector<uint8_t> &data) {
    if (data.empty() || data.size() % AES_BLOCK_LEN != 0) {
        return false;
    }
    uint8_t padLen = data.back();
    if (padLen == 0 || padLen > AES_BLOCK_LEN || padLen > data.size()) {
        return false;
    }
    for (size_t i = data.size() - padLen; i < data.size(); i++) {
        if (data[i] != padLen) {
            return false;
        }
    }
    data.resize(data.size() - padLen);
    return true;
}

std::array<uint8_t, 32> sha256(const uint8_t *data, size_t len) {
    std::array<uint8_t, 32> digest{};
    mbedtls_sha256_context ctx;
    mbedtls_sha256_init(&ctx);
    mbedtls_sha256_starts(&ctx, 0 /* SHA-256, not SHA-224 */);
    mbedtls_sha256_update(&ctx, data, len);
    mbedtls_sha256_finish(&ctx, digest.data());
    mbedtls_sha256_free(&ctx);
    return digest;
}

} // namespace

AesKey deriveKey(const String &passphrase) {
    auto digest = sha256(reinterpret_cast<const uint8_t *>(passphrase.c_str()), passphrase.length());
    AesKey key{};
    std::copy(digest.begin(), digest.end(), key.begin());
    return key;
}

EncryptedBody encryptMessage(const String &plaintext, const AesKey &key) {
    auto tag = sha256(reinterpret_cast<const uint8_t *>(plaintext.c_str()), plaintext.length());

    std::vector<uint8_t> body;
    body.reserve(INTEGRITY_TAG_LEN + plaintext.length());
    body.insert(body.end(), tag.begin(), tag.begin() + INTEGRITY_TAG_LEN);
    body.insert(body.end(), plaintext.c_str(), plaintext.c_str() + plaintext.length());

    std::vector<uint8_t> padded = pkcs7Pad(body);

    EncryptedBody out;
    esp_fill_random(out.iv, AES_IV_LEN);
    out.ciphertext.resize(padded.size());

    uint8_t ivWork[AES_IV_LEN];
    memcpy(ivWork, out.iv, AES_IV_LEN);

    mbedtls_aes_context aes;
    mbedtls_aes_init(&aes);
    mbedtls_aes_setkey_enc(&aes, key.data(), AES_KEY_LEN * 8);
    mbedtls_aes_crypt_cbc(&aes, MBEDTLS_AES_ENCRYPT, padded.size(), ivWork, padded.data(), out.ciphertext.data());
    mbedtls_aes_free(&aes);

    return out;
}

bool decryptMessage(const uint8_t iv[AES_IV_LEN], const std::vector<uint8_t> &ciphertext,
                     const AesKey &key, String &outPlaintext) {
    if (ciphertext.empty() || ciphertext.size() % AES_BLOCK_LEN != 0) {
        return false;
    }

    std::vector<uint8_t> padded(ciphertext.size());
    uint8_t ivWork[AES_IV_LEN];
    memcpy(ivWork, iv, AES_IV_LEN);

    mbedtls_aes_context aes;
    mbedtls_aes_init(&aes);
    mbedtls_aes_setkey_dec(&aes, key.data(), AES_KEY_LEN * 8);
    mbedtls_aes_crypt_cbc(&aes, MBEDTLS_AES_DECRYPT, ciphertext.size(), ivWork, ciphertext.data(), padded.data());
    mbedtls_aes_free(&aes);

    if (!pkcs7Unpad(padded)) {
        return false;
    }
    if (padded.size() < INTEGRITY_TAG_LEN) {
        return false;
    }

    std::vector<uint8_t> plaintextBytes(padded.begin() + INTEGRITY_TAG_LEN, padded.end());
    auto expectedTag = sha256(plaintextBytes.data(), plaintextBytes.size());
    if (!std::equal(padded.begin(), padded.begin() + INTEGRITY_TAG_LEN, expectedTag.begin())) {
        // Either a corrupted message or -- the common case -- this node's
        // channel key doesn't match the sender's.
        return false;
    }

    plaintextBytes.push_back(0); // NUL-terminate: Arduino String has no (buf, len) ctor
    outPlaintext = String(reinterpret_cast<const char *>(plaintextBytes.data()));
    return true;
}

} // namespace crypto
