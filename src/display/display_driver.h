#pragma once

// Wires TFT_eSPI (ILI9341) + XPT2046 resistive touch into LVGL's v9
// display/indev API. Call begin() once in setup(), then tick() every
// loop() iteration.
namespace display {

void begin();
void tick();

// Uncalibrated ADC touch reading, for the calibration wizard -- bypasses
// the min/max mapping applied to LVGL's own pointer input.
bool readRawTouch(int &rawX, int &rawY);

// Updates the live raw-ADC-to-screen-pixel mapping used by LVGL's touch
// input (see setup_wizard / screen_calibration and settings_store).
void setCalibration(int xMin, int xMax, int yMin, int yMax);

} // namespace display
