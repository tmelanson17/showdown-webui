#pragma once

#include <boost/asio.hpp>

#include "user_login.h"
#include "showdown_state_machine.h"

namespace ps_client {
class LoginState : public ShowdownClientStateMachine::StateAction {
public:
    LoginState(boost::asio::io_context& ioc) : user_login_(ioc) {}
    ShowdownClientStateMachine::StateEnumType NextState(ShowdownClientStateMachine::ContextType* context) override {

        // Check if the context Message header is "challstr".
        if (context->last_message.header == "challstr") {
            User::UserInfo user_info{
                kUsername,
                kPassword,
                context->last_message.contents,
            };
            // Send the login information to the server.
            // TODO: Make the string concatenation more efficient.
            context->write( 
                std::string("/trn ") + std::string(kUsername) + std::string(",0,") + user_login_.Login(user_info));
        
            return ShowdownClientStateEnum::kJoinLobby;
        }

        return ShowdownClientStateEnum::kLoggingIn;
    }
private:
    static constexpr std::string_view kUsername{"bot4352"};
    static constexpr std::string_view kPassword{"p"};
    User user_login_;
};
}
