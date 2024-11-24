#pragma once

#include <boost/asio.hpp>

#include "showdown_state_machine.h"
#include "user_login.h"

namespace ps_client {
class LoginState : public ShowdownClientStateMachine::StateAction {
 public:
  LoginState(boost::asio::io_context& ioc) : user_login_(ioc) {}
  ShowdownClientStateMachine::StateEnumType NextState(
      ShowdownClientStateMachine::ContextType* context) override {
    if (!std::holds_alternative<WebsocketMessage>(context->last_message)) {
      std::cerr << "Expected a WebsocketMessage in LoginState." << std::endl;
      return ShowdownClientStateEnum::kLoggingIn;
    }

    // Check if the context Message header is "challstr".
    WebsocketMessage message =
        std::get<WebsocketMessage>(context->last_message);
    if (message.header == "challstr") {
      User::UserInfo user_info{
          kUsername,
          kPassword,
          message.contents,
      };
      // Send the login information to the server.
      // TODO: Make the string concatenation more efficient.
      std::string login_message = "/trn ";
      login_message += kUsername;
      login_message += ",0,";
      login_message += user_login_.Login(user_info);
      context->socket_write(login_message);

      return ShowdownClientStateEnum::kJoinLobby;
    }

    return ShowdownClientStateEnum::kLoggingIn;
  }

 private:
  static constexpr std::string_view kUsername{"bot4352"};
  static constexpr std::string_view kPassword{"p"};
  User user_login_;
};
}  // namespace ps_client
