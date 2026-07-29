#pragma once

// Text-based interface over Serial, mirroring what the touchscreen UI does
// (view/send messages, set handle/name/channel key, view mesh topology) --
// for devices without a screen, or as a secondary interface on ones with one.
namespace serial_console {

void begin();
void tick(); // call every loop(); non-blocking, drains Serial byte by byte

} // namespace serial_console
