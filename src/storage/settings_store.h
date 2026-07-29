#pragma once

#include <Arduino.h>

// Persists device configuration -- touch calibration, user handle/name,
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

String getHandle();
String getName();
String getNetworkKey();
void setHandle(const String &handle);
void setName(const String &name);
void setNetworkKey(const String &key);

TouchCalibration getCalibration();
void setCalibration(const TouchCalibration &cal);

} // namespace settings
