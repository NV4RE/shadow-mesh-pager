#include "ui_manager.h"

#include <lvgl.h>

#include "screen_compose.h"
#include "screen_messages.h"
#include "screen_settings.h"
#include "screen_topology.h"

namespace ui {

namespace {

lv_obj_t *content = nullptr;
lv_obj_t *navBar = nullptr;

lv_obj_t *messagesScreen = nullptr;
lv_obj_t *composeScreen = nullptr;
lv_obj_t *topologyScreen = nullptr;
lv_obj_t *settingsScreen = nullptr;

void navButtonEventCb(lv_event_t *e) {
    auto target = static_cast<Screen>(reinterpret_cast<intptr_t>(lv_event_get_user_data(e)));
    show(target);
}

void addNavButton(lv_obj_t *bar, const char *label, Screen target) {
    lv_obj_t *btn = lv_button_create(bar);
    lv_obj_set_flex_grow(btn, 1);
    lv_obj_add_event_cb(btn, navButtonEventCb, LV_EVENT_CLICKED,
                         reinterpret_cast<void *>(static_cast<intptr_t>(target)));

    lv_obj_t *lbl = lv_label_create(btn);
    lv_label_set_text(lbl, label);
    lv_obj_center(lbl);
}

} // namespace

void begin() {
    lv_obj_t *root = lv_screen_active();
    lv_obj_set_flex_flow(root, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(root, 0, 0);

    content = lv_obj_create(root);
    lv_obj_set_width(content, LV_PCT(100));
    lv_obj_set_flex_grow(content, 1);
    lv_obj_set_style_pad_all(content, 4, 0);

    navBar = lv_obj_create(root);
    lv_obj_set_width(navBar, LV_PCT(100));
    lv_obj_set_height(navBar, 40);
    lv_obj_set_flex_flow(navBar, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_all(navBar, 2, 0);
    lv_obj_set_style_pad_column(navBar, 2, 0);

    addNavButton(navBar, "Msgs", Screen::Messages);
    addNavButton(navBar, "Send", Screen::Compose);
    addNavButton(navBar, "Map", Screen::Topology);
    addNavButton(navBar, "Setup", Screen::Settings);

    messagesScreen = screen_messages::create(content);
    composeScreen = screen_compose::create(content);
    topologyScreen = screen_topology::create(content);
    settingsScreen = screen_settings::create(content);

    show(Screen::Messages);
}

void show(Screen s) {
    lv_obj_add_flag(messagesScreen, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(composeScreen, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(topologyScreen, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(settingsScreen, LV_OBJ_FLAG_HIDDEN);

    switch (s) {
        case Screen::Messages:
            lv_obj_clear_flag(messagesScreen, LV_OBJ_FLAG_HIDDEN);
            screen_messages::refresh();
            break;
        case Screen::Compose:
            lv_obj_clear_flag(composeScreen, LV_OBJ_FLAG_HIDDEN);
            break;
        case Screen::Topology:
            lv_obj_clear_flag(topologyScreen, LV_OBJ_FLAG_HIDDEN);
            screen_topology::refresh();
            break;
        case Screen::Settings:
            lv_obj_clear_flag(settingsScreen, LV_OBJ_FLAG_HIDDEN);
            screen_settings::refresh();
            break;
    }
}

} // namespace ui
