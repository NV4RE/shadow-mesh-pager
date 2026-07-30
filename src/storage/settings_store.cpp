#include "settings_store.h"

#include <Preferences.h>

#include "../config.h"

namespace settings {

namespace {
Preferences prefs;
} // namespace

void begin() { prefs.begin(NVS_NAMESPACE, false); }

bool isSetupComplete() { return prefs.getBool("setup_done", false); }
void markSetupComplete() { prefs.putBool("setup_done", true); }

String getName() { return prefs.getString("name", ""); }
String getNetworkKey() { return prefs.getString("netkey", ""); }

void setName(const String &name) { prefs.putString("name", name); }
void setNetworkKey(const String &key) { prefs.putString("netkey", key); }

uint32_t getLedColor() { return prefs.getUInt("led_color", 0); }
void setLedColor(uint32_t rgb) { prefs.putUInt("led_color", rgb); }

int8_t getWifiGain() { return prefs.getChar("wifi_gain", DEFAULT_WIFI_GAIN_RAW); }
void setWifiGain(int8_t rawPower) { prefs.putChar("wifi_gain", rawPower); }

TouchCalibration getCalibration() {
    TouchCalibration cal;
    cal.xMin = prefs.getInt("cal_xmin", cal.xMin);
    cal.xMax = prefs.getInt("cal_xmax", cal.xMax);
    cal.yMin = prefs.getInt("cal_ymin", cal.yMin);
    cal.yMax = prefs.getInt("cal_ymax", cal.yMax);
    return cal;
}

void setCalibration(const TouchCalibration &cal) {
    prefs.putInt("cal_xmin", cal.xMin);
    prefs.putInt("cal_xmax", cal.xMax);
    prefs.putInt("cal_ymin", cal.yMin);
    prefs.putInt("cal_ymax", cal.yMax);
}

void factoryReset() {
    prefs.clear();
    prefs.end();
    ESP.restart();
}

} // namespace settings
