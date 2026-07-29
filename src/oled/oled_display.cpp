#include "oled_display.h"

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Arduino.h>
#include <Wire.h>

#include "../config.h"
#include "../message/message.h"
#include "../network/mesh_manager.h"

namespace oled_display {

namespace {

Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire, OLED_RST_PIN);

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

String bodyText(const Message &msg) {
    if (!msg.decryptable) {
        return "<undecryptable>";
    }
    if (msg.type == MessageType::Emoji) {
        return "[" + String(emojiGlyph(msg.body)) + "]";
    }
    return msg.body;
}

void redraw() {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);

    const auto &history = meshManager.history();
    if (history.empty()) {
        display.println("Waiting for messages...");
    } else {
        // 6px-wide/8px-tall glyphs at text size 1 -> ~8 lines fit on 64px
        // tall; keep one line of margin.
        constexpr size_t MAX_LINES = 7;
        size_t start = history.size() > MAX_LINES ? history.size() - MAX_LINES : 0;
        for (size_t i = start; i < history.size(); i++) {
            display.printf("%s: %s\n", senderLabel(history[i]).c_str(), bodyText(history[i]).c_str());
        }
    }

    display.display();
}

} // namespace

void begin() {
    pinMode(OLED_VEXT_PIN, OUTPUT);
    digitalWrite(OLED_VEXT_PIN, LOW); // enable the OLED's power rail

    pinMode(OLED_RST_PIN, OUTPUT);
    digitalWrite(OLED_RST_PIN, LOW);
    delay(20);
    digitalWrite(OLED_RST_PIN, HIGH);

    Wire.begin(OLED_SDA_PIN, OLED_SCL_PIN);

    if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_I2C_ADDR)) {
        return; // nothing more we can do without the panel responding
    }

    redraw();
    meshManager.onMessage([](const Message &) { redraw(); });
}

} // namespace oled_display
