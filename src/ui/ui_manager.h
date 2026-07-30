#pragma once

#include <Arduino.h>

namespace ui {

enum class Screen { Messages, Compose, Topology, Led, Settings };

// Builds all screens (once) and shows Messages.
void begin();
void show(Screen s);

// Live feedback for BOOT0 Morse input (see input/morse_input.h): shows a
// thin status bar above the nav bar, visible regardless of which screen is
// active. Hidden automatically once both strings are empty (idle).
void setMorseStatus(const String &decoded, const String &symbols);

} // namespace ui
