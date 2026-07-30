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
lv_obj_t *gainDropdown = nullptr;

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

String gainOptionsText() {
    String opts;
    for (size_t i = 0; i < WIFI_GAIN_TABLE_SIZE; i++) {
        if (i > 0) {
            opts += "\n";
        }
        opts += WIFI_GAIN_TABLE[i].label;
    }
    return opts;
}

void gainChangedEventCb(lv_event_t *e) {
    uint32_t sel = lv_dropdown_get_selected(gainDropdown);
    if (sel >= WIFI_GAIN_TABLE_SIZE) {
        return;
    }
    int8_t raw = WIFI_GAIN_TABLE[sel].rawPower;
    settings::setWifiGain(raw);
    meshManager.setTxPower(raw);
}

void factoryResetCancelCb(lv_event_t *e) {
    lv_msgbox_close(static_cast<lv_obj_t *>(lv_event_get_user_data(e)));
}

void factoryResetConfirmCb(lv_event_t *e) {
    settings::factoryReset(); // wipes NVS and reboots -- does not return
}

void factoryResetEventCb(lv_event_t *e) {
    lv_obj_t *mbox = lv_msgbox_create(nullptr); // NULL parent -> modal, on top of everything
    lv_msgbox_add_title(mbox, "Factory reset?");
    lv_msgbox_add_text(mbox, "Wipes name, channel key, gain, LED color, and touch calibration, "
                              "then reboots. This cannot be undone.");
    lv_obj_t *cancelBtn = lv_msgbox_add_footer_button(mbox, "Cancel");
    lv_obj_add_event_cb(cancelBtn, factoryResetCancelCb, LV_EVENT_CLICKED, mbox);
    lv_obj_t *resetBtn = lv_msgbox_add_footer_button(mbox, "Reset");
    lv_obj_add_event_cb(resetBtn, factoryResetConfirmCb, LV_EVENT_CLICKED, nullptr);
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

    lv_obj_t *gainLabel = lv_label_create(root);
    lv_label_set_text(gainLabel, "WiFi gain (TX power):");

    gainDropdown = lv_dropdown_create(root);
    lv_obj_set_width(gainDropdown, LV_PCT(100));
    lv_dropdown_set_options(gainDropdown, gainOptionsText().c_str());
    lv_obj_add_event_cb(gainDropdown, gainChangedEventCb, LV_EVENT_VALUE_CHANGED, nullptr);

    lv_obj_t *resetBtn = lv_button_create(root);
    lv_obj_t *resetLbl = lv_label_create(resetBtn);
    lv_label_set_text(resetLbl, "Factory reset");
    lv_obj_center(resetLbl);
    lv_obj_add_event_cb(resetBtn, factoryResetEventCb, LV_EVENT_CLICKED, nullptr);

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

    int8_t rawGain = settings::getWifiGain();
    for (size_t i = 0; i < WIFI_GAIN_TABLE_SIZE; i++) {
        if (WIFI_GAIN_TABLE[i].rawPower == rawGain) {
            lv_dropdown_set_selected(gainDropdown, i);
            break;
        }
    }

    lv_label_set_text_fmt(statusLabel, "Channel key: %s", meshManager.hasChannelKey() ? "set" : "not set");
}

} // namespace screen_settings
