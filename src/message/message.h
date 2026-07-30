#pragma once

#include <Arduino.h>
#include <cstddef>

#include "../crypto/aes_channel.h"

struct Message {
    String id;                       // UUID v4, dedup key
    uint32_t from = 0;                // painlessMesh node id of the true origin
    uint32_t ts = 0;                  // millis() at origin, display ordering only
    bool decryptable = false;         // false => this node's channel key didn't match
    String body;                      // decrypted plaintext

    // Sender's display name, plaintext on the wire by design so it's
    // visible even to nodes that can't decrypt the body.
    String name;

    // Generates a fresh id and serializes to the wire JSON envelope,
    // encrypting `body` with the given channel key. `from`/`ts`/`body` must
    // already be set on `msg`.
    static String toWireJson(Message &msg, const crypto::AesKey &key);

    // Parses a received wire JSON envelope. Structural fields (id/from/ts)
    // are always filled in; `decryptable`/`body` reflect whether this node's
    // channel key matched. Returns false only on a malformed envelope.
    static bool fromWireJson(const String &json, const crypto::AesKey &key, Message &out);
};

// A canned phrase for quick sending without typing -- picked from the CYD's
// compose screen or sent by number via the serial console's /preset command.
// Sent as an ordinary message body verbatim; there's no separate wire
// representation for "this was a preset."
struct PresetEntry {
    const char *text;
};

extern const PresetEntry PRESET_TABLE[];
extern const size_t PRESET_TABLE_SIZE;

// Random UUID v4 string, used as the message dedup id.
String generateMessageId();
