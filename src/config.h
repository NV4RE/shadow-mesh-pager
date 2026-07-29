#pragma once

#include <cstdint>

// --- Persistent settings (NVS namespace) ---
#define NVS_NAMESPACE "worldend"

// --- painlessMesh physical network ---
// One fixed, shared mesh so any two devices can relay traffic for each other
// regardless of which AES channel key they hold (digipeater model). The AES
// passphrase entered in the settings screen is the real "sub network" secret.
#define MESH_SSID "worldend-mesh"
#define MESH_PASSWORD "worldend-mesh-relay"
#define MESH_PORT 5555

// --- Message history / dedup (in-RAM only, per spec) ---
#define MESSAGE_HISTORY_CAPACITY 64
#define SEEN_ID_RING_CAPACITY 64

// --- Crypto ---
#define AES_KEY_LEN 32   // AES-256
#define AES_IV_LEN 16
#define AES_BLOCK_LEN 16
#define INTEGRITY_TAG_LEN 4

// --- Display ---
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240
#define LVGL_TICK_PERIOD_MS 5

// --- Status LED (discrete 5050 RGB LED behind the panel, active-low: LOW
// turns a channel ON, HIGH turns it OFF) -- esp32dev (CYD) target only ---
#define RGB_LED_RED_PIN 4
#define RGB_LED_GREEN_PIN 16
#define RGB_LED_BLUE_PIN 17
#define RGB_LED_RED_CHANNEL 0
#define RGB_LED_GREEN_CHANNEL 1
#define RGB_LED_BLUE_CHANNEL 2
#define RGB_LED_PWM_FREQ_HZ 5000
#define RGB_LED_PWM_RESOLUTION_BITS 8

// --- Onboard SSD1306 OLED (I2C) -- heltec_wifi_lora_32_v2 target only.
// Pins per the board's own pins_arduino.h variant; Vext must be pulled LOW
// to power the OLED before it will respond on I2C. ---
#define OLED_SDA_PIN 4
#define OLED_SCL_PIN 15
#define OLED_RST_PIN 16
#define OLED_VEXT_PIN 21
#define OLED_WIDTH 128
#define OLED_HEIGHT 64
#define OLED_I2C_ADDR 0x3C
