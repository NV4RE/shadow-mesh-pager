#include "serial_console.h"

#include <Arduino.h>

#include "../led/rgb_led.h"
#include "../message/message.h"
#include "../network/mesh_manager.h"
#include "../storage/settings_store.h"

namespace serial_console {

namespace {

String lineBuf;

const char *emojiGlyph(const String &code) {
    for (size_t i = 0; i < EMOJI_TABLE_SIZE; i++) {
        if (code == EMOJI_TABLE[i].code) {
            return EMOJI_TABLE[i].glyph;
        }
    }
    return code.c_str();
}

String senderLabel(const Message &msg) {
    if (msg.from == meshManager.selfId()) {
        return "me";
    }
    if (msg.name.length() > 0) {
        return msg.name;
    }
    char buf[10];
    snprintf(buf, sizeof(buf), "%08X", static_cast<unsigned int>(msg.from));
    return String(buf);
}

void printMessage(const Message &msg) {
    if (!msg.decryptable) {
        Serial.printf("[msg] %s: <undecryptable -- different channel key>\n", senderLabel(msg).c_str());
    } else if (msg.type == MessageType::Emoji) {
        Serial.printf("[msg] %s: [%s]\n", senderLabel(msg).c_str(), emojiGlyph(msg.body));
    } else {
        Serial.printf("[msg] %s: %s\n", senderLabel(msg).c_str(), msg.body.c_str());
    }
}

void printHelp() {
    Serial.println(F("World End SMS serial console. Commands:"));
    Serial.println(F("  /help              show this help"));
    Serial.println(F("  /whoami            show your node id / name / channel status"));
    Serial.println(F("  /name <text>       set your display name (shown next to your messages)"));
    Serial.println(F("  /key <text>        set the shared channel (network) key"));
    Serial.println(F("  /led <hex>          set the status LED color, e.g. /led ff8800"));
    Serial.println(F("  /emojis            list available emoji codes"));
    Serial.println(F("  /emoji <code>      send an emoji, e.g. /emoji :wave:"));
    Serial.println(F("  /topology          list known mesh nodes"));
    Serial.println(F("  /history           reprint recent message history"));
    Serial.println(F("  (anything else)    sent as a text message"));
}

void printWhoami() {
    Serial.printf("[me] node=%08X name=\"%s\" channel_key=%s\n",
                  static_cast<unsigned int>(meshManager.selfId()), settings::getName().c_str(),
                  meshManager.hasChannelKey() ? "set" : "not set");
}

void printTopology() {
    char selfBuf[10];
    snprintf(selfBuf, sizeof(selfBuf), "%08X", static_cast<unsigned int>(meshManager.selfId()));
    String selfName = meshManager.selfName();
    Serial.printf("[net] me: %s%s\n", selfBuf,
                   selfName.length() > 0 ? (" (" + selfName + ")").c_str() : "");

    for (uint32_t id : meshManager.nodeIds()) {
        if (id == meshManager.selfId()) {
            continue;
        }
        char idBuf[10];
        snprintf(idBuf, sizeof(idBuf), "%08X", static_cast<unsigned int>(id));
        String n = meshManager.nameForNode(id);
        Serial.printf("[net] %s%s -- %s\n", idBuf, n.length() > 0 ? (" (" + n + ")").c_str() : "",
                       meshManager.isConnected(id) ? "direct" : "via relay");
    }
}

void printHistory() {
    for (const auto &msg : meshManager.history()) {
        printMessage(msg);
    }
}

void printEmojiTable() {
    for (size_t i = 0; i < EMOJI_TABLE_SIZE; i++) {
        Serial.printf("  %s  (%s)\n", EMOJI_TABLE[i].code, EMOJI_TABLE[i].glyph);
    }
}

void handleCommand(const String &line) {
    if (line.length() == 0) {
        return;
    }

    if (!line.startsWith("/")) {
        meshManager.sendMessage(MessageType::Text, line);
        Serial.println("[ok] sent");
        return;
    }

    int sp = line.indexOf(' ');
    String cmd = sp == -1 ? line : line.substring(0, sp);
    String arg = sp == -1 ? String() : line.substring(sp + 1);
    cmd.toLowerCase();

    if (cmd == "/help") {
        printHelp();
    } else if (cmd == "/whoami") {
        printWhoami();
    } else if (cmd == "/name") {
        settings::setName(arg);
        settings::markSetupComplete();
        meshManager.setIdentity(arg);
        Serial.printf("[ok] name set to \"%s\"\n", arg.c_str());
    } else if (cmd == "/key") {
        settings::setNetworkKey(arg);
        settings::markSetupComplete();
        meshManager.setChannelKey(arg);
        Serial.println("[ok] channel key set");
    } else if (cmd == "/led") {
        if (arg.length() == 0 || arg.length() > 6) {
            Serial.println("[err] usage: /led <hex, e.g. ff8800>");
        } else {
            uint32_t rgb = strtoul(arg.c_str(), nullptr, 16);
            settings::setLedColor(rgb);
            rgb_led::setColorHex(rgb);
            Serial.printf("[ok] LED set to #%06X\n", static_cast<unsigned int>(rgb));
        }
    } else if (cmd == "/emojis") {
        printEmojiTable();
    } else if (cmd == "/emoji") {
        bool found = false;
        for (size_t i = 0; i < EMOJI_TABLE_SIZE; i++) {
            if (arg == EMOJI_TABLE[i].code) {
                found = true;
                break;
            }
        }
        if (!found) {
            Serial.printf("[err] unknown emoji code \"%s\" -- try /emojis\n", arg.c_str());
        } else {
            meshManager.sendMessage(MessageType::Emoji, arg);
            Serial.println("[ok] sent");
        }
    } else if (cmd == "/topology") {
        printTopology();
    } else if (cmd == "/history") {
        printHistory();
    } else {
        Serial.printf("[err] unknown command \"%s\" -- try /help\n", cmd.c_str());
    }
}

} // namespace

void begin() {
    Serial.println(F("[console] World End SMS serial console ready. Type /help for commands."));
    meshManager.onMessage(printMessage);
}

void tick() {
    while (Serial.available() > 0) {
        char c = static_cast<char>(Serial.read());
        if (c == '\n' || c == '\r') {
            if (lineBuf.length() > 0) {
                handleCommand(lineBuf);
                lineBuf = "";
            }
        } else {
            lineBuf += c;
        }
    }
}

} // namespace serial_console
