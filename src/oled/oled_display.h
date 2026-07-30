#pragma once

#include <Arduino.h>

// Read-only message view on the Heltec board's onboard SSD1306 OLED --
// sending and configuration happen over the serial console
// (see console/serial_console.h) or the BOOT0 Morse input
// (see input/morse_input.h), since this board has no touch input.
namespace oled_display {

void begin();

// Live feedback for BOOT0 Morse input: overlays the in-progress decode on
// the bottom line, in place of one line of message history. Both strings
// empty => idle, no overlay.
void setMorseStatus(const String &decoded, const String &symbols);

} // namespace oled_display
