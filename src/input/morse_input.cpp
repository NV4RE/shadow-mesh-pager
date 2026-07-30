#include "morse_input.h"

#include <vector>

#include "../config.h"
#include "../message/message.h"
#include "../network/mesh_manager.h"

namespace morse_input {

namespace {

struct MorsePattern {
    const char *pattern;
    char letter;
};

// International Morse code, A-Z and 0-9. Anything else (unmatched pattern)
// is logged to serial and dropped rather than corrupting the message.
const MorsePattern MORSE_TABLE[] = {
    {".-", 'A'},   {"-...", 'B'}, {"-.-.", 'C'}, {"-..", 'D'},  {".", 'E'},
    {"..-.", 'F'}, {"--.", 'G'},  {"....", 'H'}, {"..", 'I'},   {".---", 'J'},
    {"-.-", 'K'},  {".-..", 'L'}, {"--", 'M'},   {"-.", 'N'},   {"---", 'O'},
    {".--.", 'P'}, {"--.-", 'Q'}, {".-.", 'R'},  {"...", 'S'},  {"-", 'T'},
    {"..-", 'U'},  {"...-", 'V'}, {".--", 'W'},  {"-..-", 'X'}, {"-.--", 'Y'},
    {"--..", 'Z'}, {"-----", '0'}, {".----", '1'}, {"..---", '2'}, {"...--", '3'},
    {"....-", '4'}, {".....", '5'}, {"-....", '6'}, {"--...", '7'}, {"---..", '8'},
    {"----.", '9'},
};
constexpr size_t MORSE_TABLE_SIZE = sizeof(MORSE_TABLE) / sizeof(MORSE_TABLE[0]);

constexpr unsigned long DEBOUNCE_MS = 20;
constexpr unsigned long MIN_PRESS_MS = 15; // shorter than this is contact bounce, not a tap

bool rawPressed_ = false;
bool debouncedPressed_ = false;
unsigned long lastEdgeMs_ = 0;      // last time the raw reading changed, for debounce
unsigned long pressStartMs_ = 0;
unsigned long lastReleaseMs_ = 0;

String symbolBuffer_;   // dots/dashes for the letter currently in progress
String decodedBuffer_;  // message text finalized so far

// Per-gap-since-release bookkeeping so each threshold only fires once.
bool letterFinalizedForGap_ = true;
bool wordSpaceAddedForGap_ = true;

std::vector<StatusCallback> listeners_;

void notifyStatus() {
    for (auto &cb : listeners_) {
        cb(decodedBuffer_, symbolBuffer_);
    }
}

char decodeSymbols(const String &symbols) {
    for (size_t i = 0; i < MORSE_TABLE_SIZE; i++) {
        if (symbols == MORSE_TABLE[i].pattern) {
            return MORSE_TABLE[i].letter;
        }
    }
    return '\0';
}

void finalizeLetter() {
    if (symbolBuffer_.length() == 0) {
        return;
    }
    char c = decodeSymbols(symbolBuffer_);
    if (c == '\0') {
        Serial.printf("[morse] unrecognized pattern \"%s\" ignored\n", symbolBuffer_.c_str());
    } else {
        decodedBuffer_ += c;
        Serial.printf("[morse] %s -> %c (so far: \"%s\")\n", symbolBuffer_.c_str(), c,
                      decodedBuffer_.c_str());
    }
    symbolBuffer_ = "";
    notifyStatus();
}

void addWordSpace() {
    if (decodedBuffer_.length() == 0 || decodedBuffer_.endsWith(" ")) {
        return;
    }
    decodedBuffer_ += ' ';
    notifyStatus();
}

void sendAndReset() {
    String toSend = decodedBuffer_;
    toSend.trim();
    if (toSend.length() > 0) {
        meshManager.sendMessage(toSend);
        Serial.printf("[morse] sent: \"%s\"\n", toSend.c_str());
    }
    decodedBuffer_ = "";
    symbolBuffer_ = "";
    notifyStatus();
}

void onPress() {
    letterFinalizedForGap_ = false;
    wordSpaceAddedForGap_ = false;
}

void onRelease(unsigned long now) {
    unsigned long pressDur = now - pressStartMs_;
    if (pressDur < MIN_PRESS_MS) {
        return; // bounce, not a real tap
    }
    symbolBuffer_ += (pressDur >= MORSE_DASH_THRESHOLD_MS) ? '-' : '.';
    lastReleaseMs_ = now;
    Serial.printf("[morse] %s\n", symbolBuffer_.c_str());
    notifyStatus();
}

} // namespace

void begin() {
    pinMode(BOOT_BUTTON_PIN, INPUT_PULLUP);
    rawPressed_ = digitalRead(BOOT_BUTTON_PIN) == LOW;
    debouncedPressed_ = rawPressed_;
    lastEdgeMs_ = millis();
}

void tick() {
    unsigned long now = millis();
    bool raw = digitalRead(BOOT_BUTTON_PIN) == LOW;

    if (raw != rawPressed_) {
        rawPressed_ = raw;
        lastEdgeMs_ = now;
    }

    if (raw != debouncedPressed_ && (now - lastEdgeMs_) >= DEBOUNCE_MS) {
        debouncedPressed_ = raw;
        if (debouncedPressed_) {
            pressStartMs_ = now;
            onPress();
        } else {
            onRelease(now);
        }
    }

    if (!debouncedPressed_ && lastReleaseMs_ != 0) {
        unsigned long sinceRelease = now - lastReleaseMs_;

        if (!letterFinalizedForGap_ && sinceRelease >= MORSE_LETTER_GAP_MS) {
            finalizeLetter();
            letterFinalizedForGap_ = true;
        }
        if (!wordSpaceAddedForGap_ && sinceRelease >= MORSE_WORD_GAP_MS) {
            addWordSpace();
            wordSpaceAddedForGap_ = true;
        }
        if (decodedBuffer_.length() > 0 && sinceRelease >= MORSE_SEND_TIMEOUT_MS) {
            sendAndReset();
            lastReleaseMs_ = 0; // nothing pending until the next tap
        }
    }
}

void onStatusChanged(StatusCallback cb) { listeners_.push_back(std::move(cb)); }

} // namespace morse_input
