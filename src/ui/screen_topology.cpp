#include "screen_topology.h"

#include <Arduino.h>

#include "../network/mesh_manager.h"

namespace screen_topology {

namespace {

lv_obj_t *summary = nullptr;
lv_obj_t *list = nullptr;

} // namespace

lv_obj_t *create(lv_obj_t *parent) {
    lv_obj_t *root = lv_obj_create(parent);
    lv_obj_set_size(root, LV_PCT(100), LV_PCT(100));
    lv_obj_set_flex_flow(root, LV_FLEX_FLOW_COLUMN);

    summary = lv_label_create(root);
    lv_label_set_text(summary, "Network Map");

    list = lv_obj_create(root);
    lv_obj_set_width(list, LV_PCT(100));
    lv_obj_set_flex_grow(list, 1);
    lv_obj_set_flex_flow(list, LV_FLEX_FLOW_COLUMN);

    meshManager.onTopologyChanged([]() { refresh(); });

    return root;
}

void refresh() {
    if (!list) {
        return;
    }
    lv_obj_clean(list);

    char selfBuf[10];
    snprintf(selfBuf, sizeof(selfBuf), "%08X", static_cast<unsigned int>(meshManager.selfId()));
    lv_label_set_text_fmt(summary, "Network Map -- me: %s", selfBuf);

    for (uint32_t id : meshManager.nodeIds()) {
        lv_obj_t *row = lv_label_create(list);
        char idBuf[10];
        snprintf(idBuf, sizeof(idBuf), "%08X", static_cast<unsigned int>(id));

        String text;
        if (id == meshManager.selfId()) {
            String h = meshManager.selfHandle();
            text = String(idBuf) + " (you" + (h.length() > 0 ? " - " + h : String("")) + ")";
        } else {
            String h = meshManager.handleForNode(id);
            String suffix = h.length() > 0 ? " (" + h + ")" : String("");
            text = String(idBuf) + suffix + " -- " + (meshManager.isConnected(id) ? "direct" : "via relay");
        }
        lv_label_set_text(row, text.c_str());
    }
}

} // namespace screen_topology
