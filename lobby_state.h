#pragma once

#include <fstream>
#include <iostream>
#include <string>
#include <variant>

#include "showdown_state_machine.h"
#include "util.h"

namespace ps_client {
class LobbyState : public ShowdownClientStateMachine::StateAction {
 public:
  void EnterState(ShowdownClientStateMachine::ContextType* context) override {
    context->socket_write("/join lobby");
  }

  ShowdownClientStateEnum NextState(
      ShowdownClientStateMachine::ContextType* context) override {
    // TODO: Maybe split the message further to check for the challenge.
    // TODO: I think this will need to be in a state after uploading the team.
    // Listen on a FIFO input fd for a team to upload.
    if (std::holds_alternative<WebsocketMessage>(context->last_message)) {
      WebsocketMessage message =
          std::get<WebsocketMessage>(context->last_message);
      if (message.header == "pm") {
        std::vector<std::string_view> split_message =
            util::SplitLine(message.contents);
        if (split_message.size() > 2 &&
            split_message[2].find("challenge") != std::string::npos) {
          std::cout << "[lobby] challenger: " << split_message[0] << std::endl;
          user_ = split_message[0].substr(1);
          received_challenge_ = true;
        }
        std::cout << "[lobby] received: " << message.contents << std::endl;
      }
    } else if (std::holds_alternative<Team>(context->last_message)) {
      Team team = std::get<Team>(context->last_message);
      context->socket_write("/utm " + team.team_as_str);
      std::cout << "Found team: " << team.team_as_str << std::endl;
      sent_team_ = true;
    }

    if (sent_team_ && received_challenge_) {
      received_challenge_ = false;
      return ShowdownClientStateEnum::kAcceptChallenge;
    }

    return ShowdownClientStateEnum::kJoinLobby;
  }

  void ExitState(ShowdownClientStateMachine::ContextType* context) override {
    context->socket_write("/accept " + user_);
  }

 private:
  std::string user_;
  bool received_challenge_ = false;
  bool sent_team_ = false;
};
}  // namespace ps_client
