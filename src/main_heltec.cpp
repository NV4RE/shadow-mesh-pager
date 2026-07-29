#include <Arduino.h>
#include <WiFi.h>

#include "console/serial_console.h"
#include "network/mesh_manager.h"
#include "oled/oled_display.h"
#include "storage/settings_store.h"

// Heltec WiFi LoRa 32 (V2) entry point: no touch panel, so there's no
// setup wizard here -- configure the device (name, channel key) via the
// serial console's /name and /key commands. The onboard OLED is read-only,
// showing incoming messages as they arrive (see oled/oled_display.cpp).
void setup() {
    Serial.begin(115200);

    settings::begin();
    meshManager.begin();

    // Default WiFi TX power draws current spikes (~300-500mA) that a lot of
    // USB ports/cables can't supply cleanly, tripping the ESP32's brownout
    // detector into a boot loop -- most common when powered straight off a
    // computer's USB port rather than a dedicated wall adapter. Knocking TX
    // power down reduces those spikes; if boot loops persist, try a
    // different USB port/cable or a proper 5V/1A+ power source.
    WiFi.setTxPower(WIFI_POWER_11dBm);

    serial_console::begin();
    oled_display::begin();

    if (settings::isSetupComplete()) {
        meshManager.setIdentity(settings::getName());
        meshManager.setChannelKey(settings::getNetworkKey());
    }
}

void loop() {
    meshManager.update();
    serial_console::tick();
}
