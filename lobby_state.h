#pragma once

#include "showdown_state_machine.h"

namespace ps_client {
class LobbyState : public ShowdownClientStateMachine::StateAction {
public:
    void EnterState(ShowdownClientStateMachine::ContextType* context) override {
        context->write("|/join lobby");
    }

    ShowdownClientStateEnum NextState(ShowdownClientStateMachine::ContextType* context) override {
        return ShowdownClientStateEnum::kJoinLobby;
    }

    void ExitState(ShowdownClientStateMachine::ContextType* context) override {}
};
} // namespace ps_client