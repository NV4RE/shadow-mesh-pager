#pragma once

namespace ui {

enum class Screen { Messages, Compose, Topology, Settings };

// Builds all four screens (once) and shows Messages.
void begin();
void show(Screen s);

} // namespace ui
