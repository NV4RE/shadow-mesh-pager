#include <Arduino.h>

#include "console/serial_console.h"
#include "display/display_driver.h"
#include "input/morse_input.h"
#include "led/rgb_led.h"
#include "network/mesh_manager.h"
#include "storage/settings_store.h"
#include "ui/setup_wizard.h"
#include "ui/ui_manager.h"

namespace {

void startMainUI() { ui::begin(); }

} // namespace

void setup() {
    Serial.begin(115200);

    display::begin();
    settings::begin();
    rgb_led::begin();
    rgb_led::setColorHex(settings::getLedColor());
    meshManager.begin();
    meshManager.setTxPower(settings::getWifiGain());
    serial_console::begin();
    morse_input::begin();
    morse_input::onStatusChanged(ui::setMorseStatus);

    if (settings::isSetupComplete()) {
        settings::TouchCalibration cal = settings::getCalibration();
        display::setCalibration(cal.xMin, cal.xMax, cal.yMin, cal.yMax);
        meshManager.setIdentity(settings::getName());
        meshManager.setChannelKey(settings::getNetworkKey());
        startMainUI();
    } else {
        // The touchscreen wizard runs regardless; on a screenless board it
        // simply never completes (no taps ever arrive), which is harmless --
        // use the serial console's /name and /key commands instead.
        setup_wizard::run(startMainUI);
    }
}

void loop() {
    meshManager.update();
    display::tick();
    serial_console::tick();
    morse_input::tick();
}
