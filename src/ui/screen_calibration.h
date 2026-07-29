#pragma once

#include <functional>

#include "../storage/settings_store.h"

// Full-screen 2-point tap touch calibration, used by the first-boot wizard
// and re-triggerable from the settings screen.
namespace screen_calibration {

using CompleteCallback = std::function<void(const settings::TouchCalibration &)>;

// Builds its own full-screen overlay on the active LVGL screen, walks the
// user through two target taps, then tears itself down and invokes
// onComplete with the derived calibration. Does not persist or apply the
// result itself -- the caller decides that (see setup_wizard / settings).
void run(CompleteCallback onComplete);

} // namespace screen_calibration
