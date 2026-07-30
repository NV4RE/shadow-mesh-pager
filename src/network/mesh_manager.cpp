#include "mesh_manager.h"

#include <WiFi.h>

// The ESP32 Arduino core's own wifi_power_t steps (dBm * 4); see
// mesh_manager.h for why these are mirrored here as plain data rather than
// used directly in the shared header.
const WifiGainOption WIFI_GAIN_TABLE[] = {
    {"19.5 dBm", 78}, {"19 dBm", 76}, {"18.5 dBm", 74}, {"17 dBm", 68},
    {"15 dBm", 60},   {"13 dBm", 52}, {"11 dBm", 44},   {"8.5 dBm", 34},
    {"7 dBm", 28},    {"5 dBm", 20},  {"2 dBm", 8},     {"-1 dBm", -4},
};
const size_t WIFI_GAIN_TABLE_SIZE = sizeof(WIFI_GAIN_TABLE) / sizeof(WIFI_GAIN_TABLE[0]);

MeshManager meshManager;

void MeshManager::begin() {
    mesh_.setDebugMsgTypes(0); // silence painlessMesh's own serial logging
    mesh_.init(MESH_SSID, MESH_PASSWORD, &scheduler_, MESH_PORT);

    mesh_.onReceive([this](uint32_t from, String &msg) { handleReceive(from, msg); });
    mesh_.onNewConnection([this](uint32_t) { handleTopologyChange(); });
    mesh_.onDroppedConnection([this](uint32_t) { handleTopologyChange(); });
    mesh_.onChangedConnections([this]() { handleTopologyChange(); });
}

void MeshManager::update() { mesh_.update(); }

void MeshManager::setChannelKey(const String &passphrase) {
    channelKey_ = crypto::deriveKey(passphrase);
    hasKey_ = passphrase.length() > 0;
}

void MeshManager::setTxPower(int8_t rawPower) {
    txPower_ = rawPower;
    WiFi.setTxPower(static_cast<wifi_power_t>(rawPower));
}

void MeshManager::setIdentity(const String &name) {
    name_ = name;
    if (name_.length() > 0) {
        nodeNames_[mesh_.getNodeId()] = name_;
    }
}

String MeshManager::nameForNode(uint32_t nodeId) const {
    auto it = nodeNames_.find(nodeId);
    return it == nodeNames_.end() ? String() : it->second;
}

void MeshManager::sendMessage(const String &body) {
    Message msg;
    msg.from = mesh_.getNodeId();
    msg.ts = static_cast<uint32_t>(millis());
    msg.body = body;
    msg.name = name_;

    String wire = Message::toWireJson(msg, channelKey_);
    mesh_.sendBroadcast(wire);

    // We authored it, so we know the plaintext regardless of hasKey_.
    msg.decryptable = true;
    remember(msg.id);
    pushHistory(msg);
    notifyMessage(msg);
}

void MeshManager::handleReceive(uint32_t from, String &json) {
    Message msg;
    if (!Message::fromWireJson(json, channelKey_, msg)) {
        return; // malformed envelope, ignore
    }
    if (alreadySeen(msg.id)) {
        return; // flood-rebroadcast duplicate
    }
    remember(msg.id);
    if (msg.name.length() > 0) {
        nodeNames_[msg.from] = msg.name;
    }
    pushHistory(msg);
    notifyMessage(msg);
}

void MeshManager::notifyMessage(const Message &msg) {
    for (const auto &listener : onMessageListeners_) {
        listener(msg);
    }
}

void MeshManager::handleTopologyChange() {
    if (onTopologyChanged_) {
        onTopologyChanged_();
    }
}

bool MeshManager::alreadySeen(const String &id) const {
    for (const auto &seen : seenIds_) {
        if (seen == id) {
            return true;
        }
    }
    return false;
}

void MeshManager::remember(const String &id) {
    seenIds_.push_back(id);
    if (seenIds_.size() > SEEN_ID_RING_CAPACITY) {
        seenIds_.pop_front();
    }
}

void MeshManager::pushHistory(const Message &msg) {
    history_.push_back(msg);
    if (history_.size() > MESSAGE_HISTORY_CAPACITY) {
        history_.pop_front();
    }
}

String MeshManager::topologyJson() { return mesh_.subConnectionJson(false); }

std::vector<uint32_t> MeshManager::nodeIds() {
    std::vector<uint32_t> ids;
    ids.push_back(mesh_.getNodeId());
    for (auto id : mesh_.getNodeList(false)) {
        ids.push_back(id);
    }
    return ids;
}

uint32_t MeshManager::selfId() { return mesh_.getNodeId(); }

bool MeshManager::isConnected(uint32_t nodeId) { return mesh_.isConnected(nodeId); }
