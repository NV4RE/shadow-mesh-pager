#include "tft_display.h"

#include <Arduino.h>
#include <TFT_eSPI.h>

#include "../message/message.h"
#include "../network/mesh_manager.h"

namespace tft_display {

namespace {

TFT_eSPI tft;

String morseDecoded_;
String morseSymbols_;

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
    return msg.body;
}

void redraw() {
    tft.fillScreen(TFT_BLACK);
    tft.setTextFont(1);
    tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setCursor(0, 0);

    bool morseActive = morseDecoded_.length() > 0 || morseSymbols_.length() > 0;

    // GLCD font is 8px tall at text size 1; leave one line of margin, minus
    // one more while the Morse status line is showing.
    const int lineHeight = tft.fontHeight();
    const size_t totalLines = tft.height() / lineHeight;
    const size_t maxLines = (morseActive ? totalLines - 2 : totalLines - 1);

    const auto &history = meshManager.history();
    if (history.empty()) {
        tft.println("Waiting for messages...");
    } else {
        size_t start = history.size() > maxLines ? history.size() - maxLines : 0;
        for (size_t i = start; i < history.size(); i++) {
            tft.printf("%s: %s\n", senderLabel(history[i]).c_str(), bodyText(history[i]).c_str());
        }
    }

    if (morseActive) {
        tft.printf("> %s%s%s\n", morseDecoded_.c_str(),
                   morseDecoded_.length() > 0 && morseSymbols_.length() > 0 ? " " : "",
                   morseSymbols_.c_str());
    }
}

} // namespace

void begin() {
    tft.begin();
    tft.setRotation(1); // landscape: 240x135

    redraw();
    meshManager.onMessage([](const Message &) { redraw(); });
}

void setMorseStatus(const String &decoded, const String &symbols) {
    morseDecoded_ = decoded;
    morseSymbols_ = symbols;
    redraw();
}

} // namespace tft_display
