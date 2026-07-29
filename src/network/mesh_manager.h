#pragma once

#include <Arduino.h>
#include <deque>
#include <functional>
#include <map>
#include <vector>

// Deliberately not including <TaskScheduler.h> directly: it defines Task/
// Scheduler methods non-inline, and every .cpp that pulled it in here used
// to get its own copy -> "multiple definition" at link time. We never call
// Task/Scheduler methods ourselves (painlessMesh owns all of that), so the
// forward-declaring <TaskSchedulerDeclarations.h> pulled in transitively by
// painlessMesh.h is enough to give `Scheduler` a complete type for the
// member below.
#include <painlessMesh.h>

#include "../config.h"
#include "../crypto/aes_channel.h"
#include "../message/message.h"

// Wraps painlessMesh: one fixed/shared physical mesh (MESH_SSID/PASSWORD)
// that every device joins and relays traffic on regardless of channel key --
// the AES passphrase (see setChannelKey) is the real "sub network" secret
// that determines what's *readable*, not what's *relayed*.
//
// painlessMesh has no per-message hop-path API (onReceive only fires on
// delivery, not on intermediate relay), so this class exposes a topology
// *snapshot* (subConnectionJson/getNodeList) for a network-map view rather
// than a per-message route trace.
class MeshManager {
public:
    using MessageCallback = std::function<void(const Message &)>;
    using TopologyCallback = std::function<void()>;

    void begin();
    void update();

    void setChannelKey(const String &passphrase);
    bool hasChannelKey() const { return hasKey_; }

    // Display name, stamped in the clear on every outgoing message (see
    // message.h) -- decoupled from AES channel readability.
    void setIdentity(const String &name);
    String selfName() const { return name_; }

    // Best-effort cache of node id -> name, learned from messages seen so
    // far (there's no separate presence/announce protocol). Empty if this
    // node hasn't been heard from yet.
    String nameForNode(uint32_t nodeId) const;

    void sendMessage(MessageType type, const String &body);

    const std::deque<Message> &history() const { return history_; }

    // Not const: painlessMesh's own accessors (getNodeId/getNodeList/
    // isConnected/subConnectionJson) aren't const-qualified.
    String topologyJson();
    std::vector<uint32_t> nodeIds();
    uint32_t selfId();
    bool isConnected(uint32_t nodeId);

    // Multiple independent listeners can subscribe (e.g. the LVGL UI and the
    // serial console both want live message notifications).
    void onMessage(MessageCallback cb) { onMessageListeners_.push_back(std::move(cb)); }
    void onTopologyChanged(TopologyCallback cb) { onTopologyChanged_ = std::move(cb); }

private:
    void handleReceive(uint32_t from, String &json);
    void notifyMessage(const Message &msg);
    void handleTopologyChange();
    bool alreadySeen(const String &id) const;
    void remember(const String &id);
    void pushHistory(const Message &msg);

    Scheduler scheduler_;
    painlessMesh mesh_;
    crypto::AesKey channelKey_{};
    bool hasKey_ = false;
    String name_;

    std::deque<Message> history_;
    std::deque<String> seenIds_;
    std::map<uint32_t, String> nodeNames_;

    std::vector<MessageCallback> onMessageListeners_;
    TopologyCallback onTopologyChanged_;
};

extern MeshManager meshManager;
