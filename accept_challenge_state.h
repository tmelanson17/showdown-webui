#pragma once

#include <fstream>
#include <iostream>
#include <variant>

#include "showdown_state_machine.h"

namespace ps_client {
class AcceptChallengeState : public ShowdownClientStateMachine::StateAction {
 public:
  ShowdownClientStateEnum NextState(
      ShowdownClientStateMachine::ContextType* context) override {
    if (std::holds_alternative<WebsocketMessage>(context->last_message)) {
      WebsocketMessage message =
          std::get<WebsocketMessage>(context->last_message);
      if (message.header == "b") {
        std::cout << "Entering battle" << std::endl;
        return ShowdownClientStateEnum::kInBattle;
      }
    }
    return ShowdownClientStateEnum::kAcceptChallenge;
  }
};
}  // namespace ps_client
