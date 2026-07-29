#include "display_driver.h"

#include <Arduino.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include <lvgl.h>

#include "../config.h"

namespace display {

namespace {

TFT_eSPI tft;

// TFT_eSPI defaults to the VSPI hardware peripheral on ESP32 (confirmed in
// its own source -- we never define USE_HSPI_PORT), so touch must use the
// other one, HSPI, with its own remapped pins. Both objects using VSPI was
// the actual bug: two SPIClass instances fighting over the same hardware
// unit's GPIO-matrix routing, corrupting TFT draws (flicker) and touch
// reads (unresponsive) every time the other one ran.
SPIClass touchSPI(HSPI);

// Deliberately NOT passing TOUCH_IRQ here. When an irq pin is given, the
// library only does an SPI read after its interrupt handler "wakes" it on a
// falling edge, and otherwise short-circuits touched()/getPoint() to stale
// data. TOUCH_IRQ (GPIO36) is one of the ESP32's input-only ADC pins with no
// internal pull-up, so that open-drain IRQ line can float and the interrupt
// never reliably re-arms -- touches would appear to work once and then stop
// responding. Omitting the irq pin makes the library always poll the panel
// over SPI on every touched()/getPoint() call, which is simple, robust, and
// plenty fast at our ~30ms poll period.
XPT2046_Touchscreen touch(XPT_CS);

// Raw ADC calibration for the resistive panel. These are sane defaults for
// this board used only until the first-boot wizard (or a settings-screen
// recalibration) overwrites them via setCalibration() -- see
// settings_store.h / screen_calibration.h.
int calXMin = 200;
int calXMax = 3700;
int calYMin = 240;
int calYMax = 3800;

lv_display_t *lvDisplay = nullptr;
lv_indev_t *lvIndev = nullptr;

// Partial-render buffer: 40 rows worth of RGB565 pixels.
uint8_t drawBuf[SCREEN_WIDTH * 40 * (LV_COLOR_DEPTH / 8)];

void flushCallback(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) {
    uint32_t w = area->x2 - area->x1 + 1;
    uint32_t h = area->y2 - area->y1 + 1;

    tft.startWrite();
    tft.setAddrWindow(area->x1, area->y1, w, h);
    // swap=true: LVGL's buffer is native-endian (LV_COLOR_16_SWAP not set),
    // TFT_eSPI wants big-endian RGB565 over SPI -- flip red/blue in the
    // panel image if this turns out backwards on real hardware.
    tft.pushColors(reinterpret_cast<uint16_t *>(px_map), w * h, true);
    tft.endWrite();

    lv_display_flush_ready(disp);
}

// LVGL v9 needs an explicit tick source registered at runtime (lv_conf.h's
// LV_TICK_CUSTOM is a v8-era macro that v9 silently ignores). millis()'s
// `unsigned long` return type isn't guaranteed to exactly match the
// required `uint32_t (*)(void)` callback signature, hence this trampoline.
uint32_t lvglTickGetCb() { return millis(); }

void touchReadCallback(lv_indev_t *indev, lv_indev_data_t *data) {
    if (touch.touched()) {
        TS_Point p = touch.getPoint();
        int x = constrain(map(p.x, calXMin, calXMax, 0, SCREEN_WIDTH - 1), 0, SCREEN_WIDTH - 1);
        int y = constrain(map(p.y, calYMin, calYMax, 0, SCREEN_HEIGHT - 1), 0, SCREEN_HEIGHT - 1);
        data->point.x = x;
        data->point.y = y;
        data->state = LV_INDEV_STATE_PRESSED;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

} // namespace

void begin() {
    tft.begin();
    tft.setRotation(1); // landscape: 320x240

    touchSPI.begin(TOUCH_CLK, TOUCH_MISO, TOUCH_MOSI, XPT_CS);
    touch.begin(touchSPI);
    touch.setRotation(1);

    lv_init();
    lv_tick_set_cb(lvglTickGetCb);

    lvDisplay = lv_display_create(SCREEN_WIDTH, SCREEN_HEIGHT);
    lv_display_set_flush_cb(lvDisplay, flushCallback);
    lv_display_set_buffers(lvDisplay, drawBuf, nullptr, sizeof(drawBuf), LV_DISPLAY_RENDER_MODE_PARTIAL);

    lvIndev = lv_indev_create();
    lv_indev_set_type(lvIndev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(lvIndev, touchReadCallback);
}

void tick() { lv_timer_handler(); }

bool readRawTouch(int &rawX, int &rawY) {
    if (!touch.touched()) {
        return false;
    }
    TS_Point p = touch.getPoint();
    rawX = p.x;
    rawY = p.y;
    return true;
}

void setCalibration(int xMin, int xMax, int yMin, int yMax) {
    calXMin = xMin;
    calXMax = xMax;
    calYMin = yMin;
    calYMax = yMax;
}

} // namespace display
