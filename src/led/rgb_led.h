#pragma once

#include <cstdint>

// Drives the single WS2812 status LED behind the panel (see config.h for
// pin/count).
namespace rgb_led {

void begin();
void setColor(uint8_t r, uint8_t g, uint8_t b);
void setColorHex(uint32_t rgb); // 0xRRGGBB

} // namespace rgb_led
