#pragma once

#include <functional>
#include <iostream>
#include <optional>
#include <string>

#include "state_machine.h"

namespace ps_client {

// Struct for decomposing a message into header and contents.
struct Message {
  std::string_view header;
  std::string_view contents;

  // Split an incoming string by '|' delimiter. The part from the first '|' 
  // to the second '|' is the header, and the rest is the contents.
  static std::optional<Message> CreateMessage(std::string_view message) {
    auto first_delim = message.find('|');
    if (first_delim == std::string::npos) {
      return std::nullopt;
    }

    auto second_delim = message.find('|', first_delim + 1);
    if (second_delim == std::string::npos) {
      return std::nullopt;
    }

    return Message{message.substr(first_delim + 1, second_delim - first_delim - 1),
                   message.substr(second_delim + 1)};
  }
};

enum class ShowdownClientStateEnum {
    kLoggingIn,
    kJoinLobby,
    kAcceptChallenge,
    kInBattle,
    kDisconnecting,
};

struct WebsocketState {
    using WriteCallback = std::function<void(const std::string&)>;
    WebsocketState(const WriteCallback& callback):
        write{callback} {}
    WebsocketState(const WebsocketState&) = default;
    WebsocketState(WebsocketState&&) = default;
    WebsocketState& operator=(const WebsocketState&) = default;
    WebsocketState& operator=(WebsocketState&&) = default;
    ~WebsocketState() = default;

    // Set the message header and contents.
    void SetMessage(std::string_view message) {
        std::optional<Message> message_or = Message::CreateMessage(message);
        if (message_or.has_value()) {
            last_message = message_or.value();
        } else {
            std::cerr << "Invalid message: " << message << std::endl;
        }
    }

    // The last received message from the server.
    Message last_message;

    // Callback for writing messages to the server.
    WriteCallback write;
};

using ShowdownClientStateMachine = state_machine::StateMachine<ShowdownClientStateEnum, WebsocketState>;

} // namespace ps_client
