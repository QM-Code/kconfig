#include "chat.hpp"
#include "server/game.hpp"
#include <string>
#include <sstream>
#include "karma/common/logging.hpp"
#include "spdlog/spdlog.h"
#include "game/engine/server_engine.hpp"
#include "plugin.hpp"

Chat::Chat(Game &game) : game(game) {

}

Chat::~Chat() {
    messages.clear();
}

void Chat::handleMessage(const ClientMsg_Chat &chatMsg) {
    auto mutableMsg = chatMsg;

    const Client *fromClient = game.getClient(mutableMsg.clientId);
    if (!fromClient) {
        spdlog::warn("Chat::handleMessage: Received chat from unknown client id {}", mutableMsg.clientId);
        return;
    }

    KARMA_TRACE("net.server", "Client: {}, Message: {}", fromClient->getName(), mutableMsg.text);
}
