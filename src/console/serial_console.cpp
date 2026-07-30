#include "serial_console.h"

#include <Arduino.h>
#include <math.h>

#ifdef HAS_RGB_LED
#include "../led/rgb_led.h"
#endif
#include "../message/message.h"
#include "../network/mesh_manager.h"
#include "../storage/settings_store.h"

namespace serial_console {

namespace {

String lineBuf;

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
    } else {
        Serial.printf("[msg] %s: %s\n", senderLabel(msg).c_str(), msg.body.c_str());
    }
}

void printGainTable() {
    for (size_t i = 0; i < WIFI_GAIN_TABLE_SIZE; i++) {
        Serial.printf("  %s%s\n", WIFI_GAIN_TABLE[i].label,
                      WIFI_GAIN_TABLE[i].rawPower == meshManager.txPower() ? "  <- current" : "");
    }
}

void printHelp() {
    Serial.println(F("World End SMS serial console. Commands:"));
    Serial.println(F("  /help              show this help"));
    Serial.println(F("  /whoami            show your node id / name / channel status"));
    Serial.println(F("  /name <text>       set your display name (shown next to your messages)"));
    Serial.println(F("  /key <text>        set the shared channel (network) key"));
#ifdef HAS_RGB_LED
    Serial.println(F("  /led <hex>          set the status LED color, e.g. /led ff8800"));
#endif
    Serial.println(F("  /gain [dBm]        show/set WiFi TX power, e.g. /gain 11 (no arg lists steps)"));
    Serial.println(F("  /presets           list preset messages with their numbers"));
    Serial.println(F("  /preset <n>        send a preset message by number, e.g. /preset 1"));
    Serial.println(F("  /topology          list known mesh nodes"));
    Serial.println(F("  /history           reprint recent message history"));
    Serial.println(F("  /factory-reset     wipe all settings and reboot (needs /factory-reset confirm)"));
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

void printPresetTable() {
    for (size_t i = 0; i < PRESET_TABLE_SIZE; i++) {
        Serial.printf("  %2u: %s\n", static_cast<unsigned>(i + 1), PRESET_TABLE[i].text);
    }
}

void handleCommand(const String &line) {
    if (line.length() == 0) {
        return;
    }

    if (!line.startsWith("/")) {
        meshManager.sendMessage(line);
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
#ifdef HAS_RGB_LED
    } else if (cmd == "/led") {
        if (arg.length() == 0 || arg.length() > 6) {
            Serial.println("[err] usage: /led <hex, e.g. ff8800>");
        } else {
            uint32_t rgb = strtoul(arg.c_str(), nullptr, 16);
            settings::setLedColor(rgb);
            rgb_led::setColorHex(rgb);
            Serial.printf("[ok] LED set to #%06X\n", static_cast<unsigned int>(rgb));
        }
#endif
    } else if (cmd == "/gain") {
        if (arg.length() == 0) {
            Serial.printf("[gain] current: %.2f dBm\n", meshManager.txPower() / 4.0f);
            printGainTable();
        } else {
            float target = arg.toFloat();
            const WifiGainOption *best = &WIFI_GAIN_TABLE[0];
            float bestDiff = fabsf(target - best->rawPower / 4.0f);
            for (size_t i = 1; i < WIFI_GAIN_TABLE_SIZE; i++) {
                float diff = fabsf(target - WIFI_GAIN_TABLE[i].rawPower / 4.0f);
                if (diff < bestDiff) {
                    bestDiff = diff;
                    best = &WIFI_GAIN_TABLE[i];
                }
            }
            settings::setWifiGain(best->rawPower);
            meshManager.setTxPower(best->rawPower);
            Serial.printf("[ok] gain set to %s\n", best->label);
        }
    } else if (cmd == "/factory-reset") {
        if (arg == "confirm") {
            Serial.println(F("[ok] factory reset -- wiping settings and rebooting"));
            settings::factoryReset();
        } else {
            Serial.println(F("[warn] this wipes name, channel key, gain, LED color, and touch "
                              "calibration, then reboots."));
            Serial.println(F("[warn] run \"/factory-reset confirm\" to proceed."));
        }
    } else if (cmd == "/presets") {
        printPresetTable();
    } else if (cmd == "/preset") {
        int n = arg.toInt();
        if (n < 1 || static_cast<size_t>(n) > PRESET_TABLE_SIZE) {
            Serial.println("[err] usage: /preset <number> -- try /presets");
        } else {
            String text = PRESET_TABLE[n - 1].text;
            meshManager.sendMessage(text);
            Serial.printf("[ok] sent preset: \"%s\"\n", text.c_str());
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
