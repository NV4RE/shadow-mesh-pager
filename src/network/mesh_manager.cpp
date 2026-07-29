#include "mesh_manager.h"

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

void MeshManager::setIdentity(const String &handle, const String &name) {
    handle_ = handle;
    name_ = name;
    if (handle_.length() > 0) {
        nodeHandles_[mesh_.getNodeId()] = handle_;
    }
}

String MeshManager::handleForNode(uint32_t nodeId) const {
    auto it = nodeHandles_.find(nodeId);
    return it == nodeHandles_.end() ? String() : it->second;
}

void MeshManager::sendMessage(MessageType type, const String &body) {
    Message msg;
    msg.from = mesh_.getNodeId();
    msg.ts = static_cast<uint32_t>(millis());
    msg.type = type;
    msg.body = body;
    msg.handle = handle_;
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
    if (msg.handle.length() > 0) {
        nodeHandles_[msg.from] = msg.handle;
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
