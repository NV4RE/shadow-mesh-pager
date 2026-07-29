#include "setup_wizard.h"

#include <Arduino.h>
#include <lvgl.h>

#include "../display/display_driver.h"
#include "../network/mesh_manager.h"
#include "../storage/settings_store.h"
#include "screen_calibration.h"

namespace setup_wizard {

namespace {

enum class Step { Handle, Name, NetworkKey };

Step step = Step::Handle;
CompleteCallback onComplete;

lv_obj_t *root = nullptr;
lv_obj_t *titleLabel = nullptr;
lv_obj_t *textarea = nullptr;
lv_obj_t *keyboard = nullptr;

String handleValue;
String nameValue;
String keyValue;

void showTextStep();

void teardown() {
    if (root) {
        lv_obj_del(root);
        root = nullptr;
    }
}

void keyboardFocusEventCb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_FOCUSED) {
        lv_obj_clear_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
        lv_keyboard_set_textarea(keyboard, textarea);
    } else if (code == LV_EVENT_DEFOCUSED) {
        lv_obj_add_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
    }
}

void finishWizard() {
    settings::setHandle(handleValue);
    settings::setName(nameValue);
    settings::setNetworkKey(keyValue);
    settings::markSetupComplete();

    meshManager.setIdentity(handleValue, nameValue);
    meshManager.setChannelKey(keyValue);

    teardown();

    CompleteCallback cb = onComplete;
    onComplete = nullptr;
    if (cb) {
        cb();
    }
}

void nextEventCb(lv_event_t *e) {
    String text = lv_textarea_get_text(textarea);

    switch (step) {
        case Step::Handle:
            handleValue = text;
            step = Step::Name;
            showTextStep();
            break;
        case Step::Name:
            nameValue = text;
            step = Step::NetworkKey;
            showTextStep();
            break;
        case Step::NetworkKey:
            keyValue = text;
            finishWizard();
            break;
    }
}

void showTextStep() {
    teardown();

    root = lv_obj_create(lv_screen_active());
    lv_obj_set_size(root, LV_PCT(100), LV_PCT(100));
    lv_obj_set_flex_flow(root, LV_FLEX_FLOW_COLUMN);

    titleLabel = lv_label_create(root);
    const char *placeholder = "";
    switch (step) {
        case Step::Handle:
            lv_label_set_text(titleLabel, "Pick a short handle (e.g. WE7)");
            placeholder = "Handle";
            break;
        case Step::Name:
            lv_label_set_text(titleLabel, "Enter your name");
            placeholder = "Name";
            break;
        case Step::NetworkKey:
            lv_label_set_text(titleLabel, "Set the shared channel key");
            placeholder = "Network key";
            break;
    }

    textarea = lv_textarea_create(root);
    lv_obj_set_width(textarea, LV_PCT(100));
    lv_textarea_set_one_line(textarea, true);
    lv_textarea_set_placeholder_text(textarea, placeholder);
    if (step == Step::NetworkKey) {
        lv_textarea_set_password_mode(textarea, true);
    }
    lv_obj_add_event_cb(textarea, keyboardFocusEventCb, LV_EVENT_FOCUSED, nullptr);
    lv_obj_add_event_cb(textarea, keyboardFocusEventCb, LV_EVENT_DEFOCUSED, nullptr);

    lv_obj_t *nextBtn = lv_button_create(root);
    lv_obj_t *btnLbl = lv_label_create(nextBtn);
    lv_label_set_text(btnLbl, step == Step::NetworkKey ? "Finish" : "Next");
    lv_obj_center(btnLbl);
    lv_obj_add_event_cb(nextBtn, nextEventCb, LV_EVENT_CLICKED, nullptr);

    keyboard = lv_keyboard_create(root);
    lv_obj_set_width(keyboard, LV_PCT(100));
    lv_obj_add_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
    lv_keyboard_set_textarea(keyboard, textarea);
}

void onCalibrationDone(const settings::TouchCalibration &cal) {
    settings::setCalibration(cal);
    display::setCalibration(cal.xMin, cal.xMax, cal.yMin, cal.yMax);

    step = Step::Handle;
    showTextStep();
}

} // namespace

void run(CompleteCallback cb) {
    onComplete = std::move(cb);
    handleValue = "";
    nameValue = "";
    keyValue = "";

    screen_calibration::run(onCalibrationDone);
}

} // namespace setup_wizard
