#include <Arduino.h>

#include "console/serial_console.h"
#include "input/morse_input.h"
#include "network/mesh_manager.h"
#include "storage/settings_store.h"
#include "tft/tft_display.h"

// TTGO/LilyGo T-Display entry point: no touch panel, so there's no setup
// wizard here -- configure the device (name, channel key) via the serial
// console's /name and /key commands. The onboard ST7789 TFT is read-only,
// showing incoming messages as they arrive (see tft/tft_display.cpp).
void setup() {
    Serial.begin(115200);

    settings::begin();
    meshManager.begin();
    meshManager.setTxPower(settings::getWifiGain());

    serial_console::begin();
    tft_display::begin();
    morse_input::begin();
    morse_input::onStatusChanged(tft_display::setMorseStatus);

    if (settings::isSetupComplete()) {
        meshManager.setIdentity(settings::getName());
        meshManager.setChannelKey(settings::getNetworkKey());
    }
}

void loop() {
    meshManager.update();
    serial_console::tick();
    morse_input::tick();
}
