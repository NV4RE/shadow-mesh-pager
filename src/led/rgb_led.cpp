#include "rgb_led.h"

#include <Arduino.h>

#include "../config.h"

namespace rgb_led {

namespace {

constexpr uint32_t DUTY_MAX = (1u << RGB_LED_PWM_RESOLUTION_BITS) - 1; // 255 at 8-bit

// Active-low: a channel is fully ON when its pin is held LOW the whole
// cycle, so brightness maps to an *inverted* duty cycle.
void writeChannel(uint8_t channel, uint8_t brightness) { ledcWrite(channel, DUTY_MAX - brightness); }

} // namespace

void begin() {
    ledcSetup(RGB_LED_RED_CHANNEL, RGB_LED_PWM_FREQ_HZ, RGB_LED_PWM_RESOLUTION_BITS);
    ledcSetup(RGB_LED_GREEN_CHANNEL, RGB_LED_PWM_FREQ_HZ, RGB_LED_PWM_RESOLUTION_BITS);
    ledcSetup(RGB_LED_BLUE_CHANNEL, RGB_LED_PWM_FREQ_HZ, RGB_LED_PWM_RESOLUTION_BITS);

    ledcAttachPin(RGB_LED_RED_PIN, RGB_LED_RED_CHANNEL);
    ledcAttachPin(RGB_LED_GREEN_PIN, RGB_LED_GREEN_CHANNEL);
    ledcAttachPin(RGB_LED_BLUE_PIN, RGB_LED_BLUE_CHANNEL);

    setColor(0, 0, 0); // off until a color is applied
}

void setColor(uint8_t r, uint8_t g, uint8_t b) {
    writeChannel(RGB_LED_RED_CHANNEL, r);
    writeChannel(RGB_LED_GREEN_CHANNEL, g);
    writeChannel(RGB_LED_BLUE_CHANNEL, b);
}

void setColorHex(uint32_t rgb) {
    setColor(static_cast<uint8_t>((rgb >> 16) & 0xFF), static_cast<uint8_t>((rgb >> 8) & 0xFF),
             static_cast<uint8_t>(rgb & 0xFF));
}

} // namespace rgb_led
