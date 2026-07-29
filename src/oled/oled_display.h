#pragma once

// Read-only message view on the Heltec board's onboard SSD1306 OLED --
// sending and configuration happen entirely over the serial console
// (see console/serial_console.h) since this board has no touch input.
namespace oled_display {

void begin();

} // namespace oled_display
