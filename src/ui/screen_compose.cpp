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
        meshManager.sendMessage(MessageType::Text, String(text));
        lv_textarea_set_text(textarea, "");
    }
}

void emojiButtonEventCb(lv_event_t *e) {
    const char *code = static_cast<const char *>(lv_event_get_user_data(e));
    meshManager.sendMessage(MessageType::Emoji, String(code));
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

    lv_obj_t *emojiRow = lv_obj_create(root);
    lv_obj_set_width(emojiRow, LV_PCT(100));
    lv_obj_set_height(emojiRow, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(emojiRow, LV_FLEX_FLOW_ROW_WRAP);

    for (size_t i = 0; i < EMOJI_TABLE_SIZE; i++) {
        lv_obj_t *btn = lv_button_create(emojiRow);
        lv_obj_set_size(btn, 50, 40);
        lv_obj_t *lbl = lv_label_create(btn);
        lv_label_set_text(lbl, EMOJI_TABLE[i].glyph);
        lv_obj_center(lbl);
        void *userData = const_cast<char *>(EMOJI_TABLE[i].code);
        lv_obj_add_event_cb(btn, emojiButtonEventCb, LV_EVENT_CLICKED, userData);
    }

    // Child of this screen's own root (not the shared content area) so it
    // hides automatically whenever the compose screen itself is hidden.
    // IGNORE_LAYOUT + explicit bottom alignment takes it out of `root`'s
    // flex-column flow: as a normal flex child it was appended after the
    // textarea/send button/emoji rows, which are already taller than the
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
