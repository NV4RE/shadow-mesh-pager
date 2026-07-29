#include "message.h"

#include <ArduinoJson.h>
#include <esp_system.h>
#include <mbedtls/base64.h>

namespace {

String base64Encode(const std::vector<uint8_t> &data) {
    size_t outLen = 0;
    mbedtls_base64_encode(nullptr, 0, &outLen, data.data(), data.size());
    std::vector<unsigned char> buf(outLen + 1, 0); // +1 for NUL terminator
    size_t written = 0;
    mbedtls_base64_encode(buf.data(), outLen, &written, data.data(), data.size());
    buf[written] = 0;
    return String(reinterpret_cast<const char *>(buf.data()));
}

std::vector<uint8_t> base64Decode(const String &text) {
    size_t outLen = 0;
    const auto *src = reinterpret_cast<const unsigned char *>(text.c_str());
    mbedtls_base64_decode(nullptr, 0, &outLen, src, text.length());
    std::vector<uint8_t> buf(outLen);
    size_t written = 0;
    mbedtls_base64_decode(buf.data(), buf.size(), &written, src, text.length());
    buf.resize(written);
    return buf;
}

const char *typeToString(MessageType type) {
    return type == MessageType::Emoji ? "emoji" : "text";
}

MessageType typeFromString(const String &s) {
    return s == "emoji" ? MessageType::Emoji : MessageType::Text;
}

} // namespace

String generateMessageId() {
    uint8_t bytes[16];
    esp_fill_random(bytes, sizeof(bytes));
    bytes[6] = (bytes[6] & 0x0F) | 0x40; // version 4
    bytes[8] = (bytes[8] & 0x3F) | 0x80; // variant 10xx

    char buf[37];
    snprintf(buf, sizeof(buf),
             "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
             bytes[0], bytes[1], bytes[2], bytes[3], bytes[4], bytes[5], bytes[6], bytes[7],
             bytes[8], bytes[9], bytes[10], bytes[11], bytes[12], bytes[13], bytes[14], bytes[15]);
    return String(buf);
}

String Message::toWireJson(Message &msg, const crypto::AesKey &key) {
    if (msg.id.isEmpty()) {
        msg.id = generateMessageId();
    }

    crypto::EncryptedBody enc = crypto::encryptMessage(msg.body, key);

    JsonDocument doc;
    doc["v"] = 1;
    doc["id"] = msg.id;
    doc["from"] = msg.from;
    doc["ts"] = msg.ts;
    doc["type"] = typeToString(msg.type);
    doc["h"] = msg.handle;
    doc["n"] = msg.name;
    doc["iv"] = base64Encode(std::vector<uint8_t>(enc.iv, enc.iv + AES_IV_LEN));
    doc["ct"] = base64Encode(enc.ciphertext);

    String out;
    serializeJson(doc, out);
    return out;
}

bool Message::fromWireJson(const String &json, const crypto::AesKey &key, Message &out) {
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, json);
    if (err) {
        return false;
    }

    if (!doc["id"].is<const char *>() || !doc["iv"].is<const char *>() || !doc["ct"].is<const char *>()) {
        return false;
    }

    out.id = doc["id"].as<String>();
    out.from = doc["from"] | 0UL;
    out.ts = doc["ts"] | 0UL;
    out.type = typeFromString(doc["type"] | "text");
    out.handle = doc["h"] | "";
    out.name = doc["n"] | "";

    std::vector<uint8_t> iv = base64Decode(doc["iv"].as<String>());
    std::vector<uint8_t> ct = base64Decode(doc["ct"].as<String>());
    if (iv.size() != AES_IV_LEN) {
        out.decryptable = false;
        return true;
    }

    String plaintext;
    out.decryptable = crypto::decryptMessage(iv.data(), ct, key, plaintext);
    out.body = out.decryptable ? plaintext : String();
    return true;
}

const EmojiEntry EMOJI_TABLE[] = {
    {":wave:", "Wave"},      {":smile:", "Smile"},    {":heart:", "Heart"},
    {":thumbsup:", "+1"},    {":thumbsdown:", "-1"},  {":fire:", "Fire"},
    {":sos:", "SOS"},        {":warning:", "Warn"},   {":check:", "OK"},
    {":cross:", "No"},       {":sun:", "Sun"},        {":rain:", "Rain"},
    {":food:", "Food"},      {":water:", "Water"},    {":medic:", "Medic"},
    {":home:", "Home"},      {":car:", "Vehicle"},    {":compass:", "Compass"},
    {":clock:", "Time"},     {":question:", "?"},
};
const size_t EMOJI_TABLE_SIZE = sizeof(EMOJI_TABLE) / sizeof(EMOJI_TABLE[0]);
