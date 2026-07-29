#pragma once

#include <Arduino.h>
#include <array>
#include <vector>

#include "../config.h"

namespace crypto {

using AesKey = std::array<uint8_t, AES_KEY_LEN>;

struct EncryptedBody {
    uint8_t iv[AES_IV_LEN];
    std::vector<uint8_t> ciphertext;
};

// Channel "sub network" key = SHA-256 of the user-entered passphrase.
AesKey deriveKey(const String &passphrase);

// AES-256-CBC of [4-byte integrity tag][PKCS7-padded plaintext], fresh
// random IV per call.
EncryptedBody encryptMessage(const String &plaintext, const AesKey &key);

// Decrypts and checks the integrity tag. Returns false (and leaves
// outPlaintext untouched) if the tag doesn't match -- which is what happens
// when the message was encrypted with a different channel key.
bool decryptMessage(const uint8_t iv[AES_IV_LEN], const std::vector<uint8_t> &ciphertext,
                     const AesKey &key, String &outPlaintext);

} // namespace crypto
