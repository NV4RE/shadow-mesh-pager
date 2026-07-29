#pragma once

#include <lvgl.h>

// "View message routing" per spec.md -- painlessMesh exposes a topology
// snapshot, not a per-message hop path (see mesh_manager.h), so this screen
// renders the current mesh network map rather than a per-message trace.
namespace screen_topology {

lv_obj_t *create(lv_obj_t *parent);
void refresh();

} // namespace screen_topology
