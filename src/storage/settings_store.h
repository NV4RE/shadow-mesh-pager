#pragma once

#include <Arduino.h>

// Persists device configuration -- touch calibration, user display name,
// network channel key, and whether first-boot setup has run -- via ESP32
// NVS (Preferences). This is separate from message history, which stays
// in-RAM only per spec.
namespace settings {

struct TouchCalibration {
    int xMin = 200;
    int xMax = 3700;
    int yMin = 240;
    int yMax = 3800;
};

void begin();

bool isSetupComplete();
void markSetupComplete();

String getName();
String getNetworkKey();
void setName(const String &name);
void setNetworkKey(const String &key);

uint32_t getLedColor(); // 0xRRGGBB, defaults to off (0)
void setLedColor(uint32_t rgb);

TouchCalibration getCalibration();
void setCalibration(const TouchCalibration &cal);

} // namespace settings
