#include "rgb_led.h"

#include <Adafruit_NeoPixel.h>

#include "../config.h"

namespace rgb_led {

namespace {
Adafruit_NeoPixel strip(WS2812_LED_COUNT, WS2812_PIN, NEO_GRB + NEO_KHZ800);
} // namespace

void begin() {
    strip.begin();
    strip.show(); // off until a color is applied
}

void setColor(uint8_t r, uint8_t g, uint8_t b) {
    for (uint16_t i = 0; i < WS2812_LED_COUNT; i++) {
        strip.setPixelColor(i, strip.Color(r, g, b));
    }
    strip.show();
}

void setColorHex(uint32_t rgb) {
    setColor(static_cast<uint8_t>((rgb >> 16) & 0xFF), static_cast<uint8_t>((rgb >> 8) & 0xFF),
             static_cast<uint8_t>(rgb & 0xFF));
}

} // namespace rgb_led
