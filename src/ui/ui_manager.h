#pragma once

namespace ui {

enum class Screen { Messages, Compose, Topology, Led, Settings };

// Builds all screens (once) and shows Messages.
void begin();
void show(Screen s);

} // namespace ui
