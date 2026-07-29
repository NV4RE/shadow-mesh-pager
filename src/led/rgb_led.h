#pragma once

#include <cstdint>

// Drives the discrete 3-channel RGB LED behind the panel via PWM (see
// config.h for pins/channels).
namespace rgb_led {

void begin();
void setColor(uint8_t r, uint8_t g, uint8_t b);
void setColorHex(uint32_t rgb); // 0xRRGGBB

} // namespace rgb_led
