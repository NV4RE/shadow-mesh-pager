/*
 * LVGL v9 configuration for the World End SMS Network firmware.
 * Only the settings we need to deviate from LVGL's built-in defaults are
 * defined here -- lv_conf_internal.h fills in everything else.
 */

#ifndef LV_CONF_H
#define LV_CONF_H

#include <stdint.h>

/* ESP32 CYD panel is 16-bit RGB565 over SPI. */
#define LV_COLOR_DEPTH 16

/* Use the ESP32's real heap (malloc/free) instead of LVGL's built-in
 * allocator, which would otherwise reserve one large static pool out of
 * BSS/DRAM up front -- that alone overflowed the ESP32's DRAM budget
 * alongside WiFi/mesh and our own globals. */
#define LV_USE_STDLIB_MALLOC LV_STDLIB_CLIB
#define LV_USE_STDLIB_STRING LV_STDLIB_CLIB
#define LV_USE_STDLIB_SPRINTF LV_STDLIB_CLIB

/* No RTOS -- we drive LVGL from the Arduino loop() via lv_timer_handler(). */
#define LV_USE_OS LV_OS_NONE

/* Ticks: LV_TICK_CUSTOM is a leftover v8-era name and does nothing in v9 --
 * LVGL v9's tick source is wired up at runtime via lv_tick_set_cb(), called
 * from display::begin() in display/display_driver.cpp. Without it,
 * lv_tick_get() never advances past 0, so no periodic lv_timer (including
 * the touch input device's own read timer) ever becomes due again after the
 * display's one forced initial draw -- this was the actual root cause of
 * touch never responding. */

#define LV_USE_LOG 0

/* Widgets used by the UI: message list, compose keyboard, topology graph. */
#define LV_USE_LABEL 1
#define LV_USE_BUTTON 1
#define LV_USE_BUTTONMATRIX 1
#define LV_USE_KEYBOARD 1
#define LV_USE_TEXTAREA 1
#define LV_USE_LIST 1
#define LV_USE_IMAGE 1
#define LV_USE_LINE 1
#define LV_USE_TABVIEW 1

#define LV_FONT_MONTSERRAT_14 1
#define LV_FONT_MONTSERRAT_16 1
#define LV_FONT_DEFAULT &lv_font_montserrat_14

#endif /* LV_CONF_H */
