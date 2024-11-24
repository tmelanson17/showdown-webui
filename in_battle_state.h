#pragma once

#include "showdown_state_machine.h"

namespace ps_client {
class InBattleState : public ShowdownClientStateMachine::StateAction {
 public:
  ShowdownClientStateEnum NextState(
      ShowdownClientStateMachine::ContextType* context) override {
    if (std::holds_alternative<CompoundWebsocketMessage>(
            context->last_message)) {
      CompoundWebsocketMessage compound_message =
          std::get<CompoundWebsocketMessage>(context->last_message);
      for (const WebsocketMessage& message : compound_message.messages) {
        // If the header is "win", go back to lobby state.
        if (message.header == "win") {
          std::cout << "Returning to lobby" << std::endl;
          return ShowdownClientStateEnum::kJoinLobby;
        } else {
          std::cout << "Sending message: " << message.header << " "
                    << message.contents << std::endl;
          // Forward to fifo_write.
          context->fifo_write("|" + std::string(message.header) + "|" +
                              std::string(message.contents));
        }
      }
    } else if (std::holds_alternative<BotCommand>(context->last_message)) {
      std::cout << "Sending command: "
                << std::get<BotCommand>(context->last_message).command
                << std::endl;
      BotCommand command = std::get<BotCommand>(context->last_message);
      context->socket_write(command.command);
    }
    return ShowdownClientStateEnum::kInBattle;
  }
};
}  // namespace ps_client
