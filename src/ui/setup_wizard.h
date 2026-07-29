#pragma once

#include <functional>

// First-boot flow: touch calibration, then handle/name/network-key entry.
// Persists everything via settings_store and applies it to the display and
// mesh manager before handing control back via onComplete (main.cpp uses
// this to then build the normal app UI).
namespace setup_wizard {

using CompleteCallback = std::function<void()>;

void run(CompleteCallback onComplete);

} // namespace setup_wizard
