#pragma once

#include <lvgl.h>

namespace screen_led {

lv_obj_t *create(lv_obj_t *parent);
void refresh();

} // namespace screen_led
