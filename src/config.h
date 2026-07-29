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
