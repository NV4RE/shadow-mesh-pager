#include "screen_compose.h"

#include <Arduino.h>
#include <cstring>

#include "../message/message.h"
#include "../network/mesh_manager.h"

namespace screen_compose {

namespace {

lv_obj_t *textarea = nullptr;
lv_obj_t *keyboard = nullptr;

void keyboardFocusEventCb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_FOCUSED) {
        lv_obj_clear_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
        lv_keyboard_set_textarea(keyboard, textarea);
    } else if (code == LV_EVENT_DEFOCUSED) {
        lv_obj_add_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
    }
}

void sendTextEventCb(lv_event_t *e) {
    const char *text = lv_textarea_get_text(textarea);
    if (text != nullptr && strlen(text) > 0) {
        meshManager.sendMessage(String(text));
        lv_textarea_set_text(textarea, "");
    }
}

void presetButtonEventCb(lv_event_t *e) {
    const char *text = static_cast<const char *>(lv_event_get_user_data(e));
    meshManager.sendMessage(String(text));
}

} // namespace

lv_obj_t *create(lv_obj_t *parent) {
    lv_obj_t *root = lv_obj_create(parent);
    lv_obj_set_size(root, LV_PCT(100), LV_PCT(100));
    lv_obj_set_flex_flow(root, LV_FLEX_FLOW_COLUMN);

    textarea = lv_textarea_create(root);
    lv_obj_set_width(textarea, LV_PCT(100));
    lv_obj_set_height(textarea, 50);
    lv_textarea_set_one_line(textarea, true);
    lv_textarea_set_placeholder_text(textarea, "Type a message...");
    lv_obj_add_event_cb(textarea, keyboardFocusEventCb, LV_EVENT_FOCUSED, nullptr);
    lv_obj_add_event_cb(textarea, keyboardFocusEventCb, LV_EVENT_DEFOCUSED, nullptr);

    lv_obj_t *sendBtn = lv_button_create(root);
    lv_obj_t *sendLbl = lv_label_create(sendBtn);
    lv_label_set_text(sendLbl, "Send");
    lv_obj_center(sendLbl);
    lv_obj_add_event_cb(sendBtn, sendTextEventCb, LV_EVENT_CLICKED, nullptr);

    // Scrollable column of full-width buttons (flex_grow fills whatever
    // space is left below the textarea/send button) rather than the fixed-
    // size grid a short emoji glyph could use -- these are full phrases, so
    // they need the width and each gets its own row.
    lv_obj_t *presetList = lv_obj_create(root);
    lv_obj_set_width(presetList, LV_PCT(100));
    lv_obj_set_flex_grow(presetList, 1);
    lv_obj_set_flex_flow(presetList, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(presetList, 2, 0);

    for (size_t i = 0; i < PRESET_TABLE_SIZE; i++) {
        lv_obj_t *btn = lv_button_create(presetList);
        lv_obj_set_width(btn, LV_PCT(100));
        lv_obj_t *lbl = lv_label_create(btn);
        lv_label_set_text(lbl, PRESET_TABLE[i].text);
        lv_obj_center(lbl);
        void *userData = const_cast<char *>(PRESET_TABLE[i].text);
        lv_obj_add_event_cb(btn, presetButtonEventCb, LV_EVENT_CLICKED, userData);
    }

    // Child of this screen's own root (not the shared content area) so it
    // hides automatically whenever the compose screen itself is hidden.
    // IGNORE_LAYOUT + explicit bottom alignment takes it out of `root`'s
    // flex-column flow: as a normal flex child it was appended after the
    // textarea/send button/preset list, which are already taller than the
    // visible content area, so it rendered scrolled off-screen -- visible
    // flag correctly cleared, but nowhere the user could see or reach it.
    keyboard = lv_keyboard_create(root);
    lv_obj_set_width(keyboard, LV_PCT(100));
    lv_obj_set_height(keyboard, LV_PCT(50));
    lv_obj_add_flag(keyboard, LV_OBJ_FLAG_IGNORE_LAYOUT);
    lv_obj_align(keyboard, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_add_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
    lv_keyboard_set_textarea(keyboard, textarea);

    return root;
}

} // namespace screen_compose
