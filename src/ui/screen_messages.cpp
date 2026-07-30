#include "screen_messages.h"

#include <Arduino.h>

#include "../message/message.h"
#include "../network/mesh_manager.h"

namespace screen_messages {

namespace {

lv_obj_t *list = nullptr;

String formatNodeId(uint32_t id) {
    char buf[10];
    snprintf(buf, sizeof(buf), "%08X", static_cast<unsigned int>(id));
    return String(buf);
}

String senderLabel(const Message &msg) {
    if (msg.from == meshManager.selfId()) {
        return "me";
    }
    if (msg.name.length() > 0) {
        return msg.name;
    }
    return formatNodeId(msg.from);
}

void addRow(const Message &msg) {
    lv_obj_t *row = lv_obj_create(list);
    lv_obj_set_width(row, LV_PCT(100));
    lv_obj_set_height(row, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(row, 4, 0);

    lv_obj_t *header = lv_label_create(row);
    lv_label_set_text_fmt(header, "%s  t+%lums", senderLabel(msg).c_str(),
                           static_cast<unsigned long>(msg.ts));

    lv_obj_t *body = lv_label_create(row);
    lv_obj_set_width(body, LV_PCT(100));
    lv_label_set_long_mode(body, LV_LABEL_LONG_WRAP);

    if (!msg.decryptable) {
        lv_label_set_text(body, "[undecryptable -- different channel key]");
    } else {
        lv_label_set_text(body, msg.body.c_str());
    }
}

} // namespace

lv_obj_t *create(lv_obj_t *parent) {
    list = lv_obj_create(parent);
    lv_obj_set_size(list, LV_PCT(100), LV_PCT(100));
    lv_obj_set_flex_flow(list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(list, 4, 0);

    meshManager.onMessage([](const Message &) { refresh(); });

    return list;
}

void refresh() {
    if (!list) {
        return;
    }
    lv_obj_clean(list);
    for (const auto &msg : meshManager.history()) {
        addRow(msg);
    }
    lv_obj_scroll_to_y(list, LV_COORD_MAX, LV_ANIM_OFF);
}

} // namespace screen_messages
