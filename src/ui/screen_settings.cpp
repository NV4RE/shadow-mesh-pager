#include "screen_settings.h"

#include <Arduino.h>

#include "../display/display_driver.h"
#include "../network/mesh_manager.h"
#include "../storage/settings_store.h"
#include "screen_calibration.h"

namespace screen_settings {

namespace {

lv_obj_t *nodeIdLabel = nullptr;
lv_obj_t *nameTextarea = nullptr;
lv_obj_t *keyTextarea = nullptr;
lv_obj_t *keyboard = nullptr;
lv_obj_t *statusLabel = nullptr;

void keyboardFocusEventCb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *target = lv_event_get_target_obj(e);
    if (code == LV_EVENT_FOCUSED) {
        lv_obj_clear_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
        lv_keyboard_set_textarea(keyboard, target);
    } else if (code == LV_EVENT_DEFOCUSED) {
        lv_obj_add_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
    }
}

void saveEventCb(lv_event_t *e) {
    String name = lv_textarea_get_text(nameTextarea);
    String key = lv_textarea_get_text(keyTextarea);

    settings::setName(name);
    settings::setNetworkKey(key);
    settings::markSetupComplete();

    meshManager.setIdentity(name);
    meshManager.setChannelKey(key);

    refresh();
}

void recalibrateEventCb(lv_event_t *e) {
    screen_calibration::run([](const settings::TouchCalibration &cal) {
        settings::setCalibration(cal);
        display::setCalibration(cal.xMin, cal.xMax, cal.yMin, cal.yMax);
    });
}

lv_obj_t *addField(lv_obj_t *parent, const char *labelText, const char *placeholder, bool masked) {
    lv_obj_t *label = lv_label_create(parent);
    lv_label_set_text(label, labelText);

    lv_obj_t *ta = lv_textarea_create(parent);
    lv_obj_set_width(ta, LV_PCT(100));
    lv_textarea_set_one_line(ta, true);
    lv_textarea_set_placeholder_text(ta, placeholder);
    if (masked) {
        lv_textarea_set_password_mode(ta, true);
    }
    lv_obj_add_event_cb(ta, keyboardFocusEventCb, LV_EVENT_FOCUSED, nullptr);
    lv_obj_add_event_cb(ta, keyboardFocusEventCb, LV_EVENT_DEFOCUSED, nullptr);
    return ta;
}

} // namespace

lv_obj_t *create(lv_obj_t *parent) {
    lv_obj_t *root = lv_obj_create(parent);
    lv_obj_set_size(root, LV_PCT(100), LV_PCT(100));
    lv_obj_set_flex_flow(root, LV_FLEX_FLOW_COLUMN);

    nodeIdLabel = lv_label_create(root);

    nameTextarea = addField(root, "Name:", "Name", false);
    keyTextarea = addField(root, "Channel key (shared with your group):", "Network key", true);

    lv_obj_t *saveBtn = lv_button_create(root);
    lv_obj_t *saveLbl = lv_label_create(saveBtn);
    lv_label_set_text(saveLbl, "Save");
    lv_obj_center(saveLbl);
    lv_obj_add_event_cb(saveBtn, saveEventCb, LV_EVENT_CLICKED, nullptr);

    lv_obj_t *recalBtn = lv_button_create(root);
    lv_obj_t *recalLbl = lv_label_create(recalBtn);
    lv_label_set_text(recalLbl, "Recalibrate touch");
    lv_obj_center(recalLbl);
    lv_obj_add_event_cb(recalBtn, recalibrateEventCb, LV_EVENT_CLICKED, nullptr);

    statusLabel = lv_label_create(root);

    // See screen_compose.cpp: IGNORE_LAYOUT + bottom alignment keeps this
    // out of root's flex-column flow, which (with the id label, 2 text
    // fields, and 2 buttons above it) is already taller than the visible
    // content area -- as a normal flex child the keyboard rendered scrolled
    // off-screen instead of visibly popping up.
    keyboard = lv_keyboard_create(root);
    lv_obj_set_width(keyboard, LV_PCT(100));
    lv_obj_set_height(keyboard, LV_PCT(50));
    lv_obj_add_flag(keyboard, LV_OBJ_FLAG_IGNORE_LAYOUT);
    lv_obj_align(keyboard, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_add_flag(keyboard, LV_OBJ_FLAG_HIDDEN);

    return root;
}

void refresh() {
    if (!nodeIdLabel) {
        return;
    }
    lv_label_set_text_fmt(nodeIdLabel, "Node ID: %08X", static_cast<unsigned int>(meshManager.selfId()));
    lv_textarea_set_text(nameTextarea, settings::getName().c_str());
    lv_textarea_set_text(keyTextarea, settings::getNetworkKey().c_str());
    lv_label_set_text_fmt(statusLabel, "Channel key: %s", meshManager.hasChannelKey() ? "set" : "not set");
}

} // namespace screen_settings
