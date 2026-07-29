#pragma once

#include <lvgl.h>

namespace screen_messages {

lv_obj_t *create(lv_obj_t *parent);
void refresh();

} // namespace screen_messages
