#include "screen_led.h"

#include "../led/rgb_led.h"
#include "../storage/settings_store.h"

// No colorwheel widget is bundled in this LVGL build (9.5.0 dropped it from
// the default widget set), so color selection is three R/G/B sliders plus a
// live preview swatch instead.
namespace screen_led {

namespace {

lv_obj_t *root = nullptr;
lv_obj_t *preview = nullptr;
lv_obj_t *sliderR = nullptr;
lv_obj_t *sliderG = nullptr;
lv_obj_t *sliderB = nullptr;

uint8_t currentR = 0;
uint8_t currentG = 0;
uint8_t currentB = 0;

void updatePreview() {
    lv_obj_set_style_bg_color(preview, lv_color_make(currentR, currentG, currentB), 0);
}

void sliderEventCb(lv_event_t *e) {
    currentR = static_cast<uint8_t>(lv_slider_get_value(sliderR));
    currentG = static_cast<uint8_t>(lv_slider_get_value(sliderG));
    currentB = static_cast<uint8_t>(lv_slider_get_value(sliderB));

    rgb_led::setColor(currentR, currentG, currentB);
    updatePreview();

    if (lv_event_get_code(e) == LV_EVENT_RELEASED) {
        uint32_t rgb = (static_cast<uint32_t>(currentR) << 16) | (static_cast<uint32_t>(currentG) << 8) | currentB;
        settings::setLedColor(rgb);
    }
}

lv_obj_t *addSlider(lv_obj_t *parent, const char *labelText) {
    lv_obj_t *label = lv_label_create(parent);
    lv_label_set_text(label, labelText);

    lv_obj_t *slider = lv_slider_create(parent);
    lv_obj_set_width(slider, LV_PCT(100));
    lv_slider_set_range(slider, 0, 255);
    lv_obj_add_event_cb(slider, sliderEventCb, LV_EVENT_VALUE_CHANGED, nullptr);
    lv_obj_add_event_cb(slider, sliderEventCb, LV_EVENT_RELEASED, nullptr);
    return slider;
}

} // namespace

lv_obj_t *create(lv_obj_t *parent) {
    root = lv_obj_create(parent);
    lv_obj_set_size(root, LV_PCT(100), LV_PCT(100));
    lv_obj_set_flex_flow(root, LV_FLEX_FLOW_COLUMN);

    lv_obj_t *title = lv_label_create(root);
    lv_label_set_text(title, "LED Color");

    preview = lv_obj_create(root);
    lv_obj_set_size(preview, LV_PCT(100), 30);
    lv_obj_remove_flag(preview, LV_OBJ_FLAG_SCROLLABLE);

    sliderR = addSlider(root, "Red");
    sliderG = addSlider(root, "Green");
    sliderB = addSlider(root, "Blue");

    return root;
}

void refresh() {
    if (!root) {
        return;
    }
    uint32_t rgb = settings::getLedColor();
    currentR = static_cast<uint8_t>((rgb >> 16) & 0xFF);
    currentG = static_cast<uint8_t>((rgb >> 8) & 0xFF);
    currentB = static_cast<uint8_t>(rgb & 0xFF);

    lv_slider_set_value(sliderR, currentR, LV_ANIM_OFF);
    lv_slider_set_value(sliderG, currentG, LV_ANIM_OFF);
    lv_slider_set_value(sliderB, currentB, LV_ANIM_OFF);
    updatePreview();
}

} // namespace screen_led
