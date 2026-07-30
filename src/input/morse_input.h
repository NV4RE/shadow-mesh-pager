#pragma once

#include <Arduino.h>
#include <functional>

// Lets the BOOT0 button (see BOOT_BUTTON_PIN, config.h) double as a Morse
// code input device: short presses are dots, long presses are dashes, and
// pauses of increasing length end a letter, end a word, or -- after a long
// enough silence -- send the message that's been tapped out so far. Shared
// across both boards (main.cpp / main_heltec.cpp each call begin()/tick()
// and hook onStatusChanged() up to their own display).
namespace morse_input {

void begin();
void tick();

// Fires whenever the in-progress buffers change (a symbol tapped, a letter
// decoded, a word space, or a send/clear). `decoded` is the message text
// finalized so far; `symbols` is the dot/dash pattern for the letter
// currently in progress. Both are empty once idle.
using StatusCallback = std::function<void(const String &decoded, const String &symbols)>;
void onStatusChanged(StatusCallback cb);

} // namespace morse_input
