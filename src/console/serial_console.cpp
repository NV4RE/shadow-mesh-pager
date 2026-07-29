#include "serial_console.h"

#include <Arduino.h>

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
    if (msg.handle.length() > 0) {
        return msg.handle;
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
    Serial.println(F("  /whoami            show your node id / handle / name / channel status"));
    Serial.println(F("  /handle <text>     set your short handle (shown next to your messages)"));
    Serial.println(F("  /name <text>       set your display name"));
    Serial.println(F("  /key <text>        set the shared channel (network) key"));
    Serial.println(F("  /emojis            list available emoji codes"));
    Serial.println(F("  /emoji <code>      send an emoji, e.g. /emoji :wave:"));
    Serial.println(F("  /topology          list known mesh nodes"));
    Serial.println(F("  /history           reprint recent message history"));
    Serial.println(F("  (anything else)    sent as a text message"));
}

void printWhoami() {
    Serial.printf("[me] node=%08X handle=\"%s\" name=\"%s\" channel_key=%s\n",
                  static_cast<unsigned int>(meshManager.selfId()), settings::getHandle().c_str(),
                  settings::getName().c_str(), meshManager.hasChannelKey() ? "set" : "not set");
}

void printTopology() {
    char selfBuf[10];
    snprintf(selfBuf, sizeof(selfBuf), "%08X", static_cast<unsigned int>(meshManager.selfId()));
    String selfHandle = meshManager.selfHandle();
    Serial.printf("[net] me: %s%s\n", selfBuf,
                   selfHandle.length() > 0 ? (" (" + selfHandle + ")").c_str() : "");

    for (uint32_t id : meshManager.nodeIds()) {
        if (id == meshManager.selfId()) {
            continue;
        }
        char idBuf[10];
        snprintf(idBuf, sizeof(idBuf), "%08X", static_cast<unsigned int>(id));
        String h = meshManager.handleForNode(id);
        Serial.printf("[net] %s%s -- %s\n", idBuf, h.length() > 0 ? (" (" + h + ")").c_str() : "",
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
    } else if (cmd == "/handle") {
        settings::setHandle(arg);
        settings::markSetupComplete();
        meshManager.setIdentity(arg, settings::getName());
        Serial.printf("[ok] handle set to \"%s\"\n", arg.c_str());
    } else if (cmd == "/name") {
        settings::setName(arg);
        settings::markSetupComplete();
        meshManager.setIdentity(settings::getHandle(), arg);
        Serial.printf("[ok] name set to \"%s\"\n", arg.c_str());
    } else if (cmd == "/key") {
        settings::setNetworkKey(arg);
        settings::markSetupComplete();
        meshManager.setChannelKey(arg);
        Serial.println("[ok] channel key set");
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
