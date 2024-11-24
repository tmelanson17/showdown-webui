#pragma once

#include <functional>
#include <iostream>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <string_view>
#include <variant>

#include "state_machine.h"
#include "util.h"

namespace ps_client {

// Struct for decomposing a message into header and contents.
struct WebsocketMessage {
  std::string_view header;
  std::string_view contents;

  // Split an incoming string by '|' delimiter. The part from the first '|'
  // to the second '|' is the header, and the rest is the contents.
  static std::optional<WebsocketMessage> CreateMessage(
      std::string_view message) {
    auto first_delim = message.find('|');
    if (first_delim == std::string::npos) {
      return std::nullopt;
    }

    auto second_delim = message.find('|', first_delim + 1);
    if (second_delim == std::string::npos) {
      return std::nullopt;
    }

    return WebsocketMessage{
        message.substr(first_delim + 1, second_delim - first_delim - 1),
        message.substr(second_delim + 1)};
  }
};

// For a battle message containing multiple WebsocketMessage.
struct CompoundWebsocketMessage {
  std::vector<WebsocketMessage> messages;

  // Determined by a line containing ">" at the beginning.
  static std::optional<CompoundWebsocketMessage> CreateCompoundMessage(
      std::string_view compount_message) {
    if (compount_message[0] != '>') {
      return std::nullopt;
    }
    // Find the first newline character.
    auto first_newline = compount_message.find('\n');
    if (first_newline == std::string::npos) {
      return std::nullopt;
    }
    // Get the compount_message after the first newline.
    std::string_view body = compount_message.substr(first_newline + 1);

    std::vector<WebsocketMessage> messages;
    std::vector<std::string_view> split_message = util::SplitLine(body, '\n');
    std::cout << "Compound message contains " << split_message.size()
              << " messages." << std::endl;
    for (const auto& line : split_message) {
      std::optional<WebsocketMessage> message_or =
          WebsocketMessage::CreateMessage(line);
      if (message_or.has_value()) {
        messages.push_back(message_or.value());
      }
    }

    return CompoundWebsocketMessage{messages};
  }
};

// For parsing the team JSON string.
struct Team {
  std::string team_as_str;

  static std::optional<Team> CreateTeam(std::string_view team) {
    // If the team is not a valid JSON string, return nullopt.
    Team result;
    try {
      nlohmann::json team_as_json = nlohmann::json::parse(team);
      result.team_as_str = team_as_json["team"];
    } catch (const nlohmann::json::parse_error& e) {
      return std::nullopt;
    }

    return result;
  }
};

// Data for representing a command from the bot.
struct BotCommand {
  std::string command;
  std::string argument;

  static std::optional<BotCommand> CreateCommand(std::string_view message) {
    auto first_space = message.find(' ');
    if (first_space == std::string::npos) {
      return std::nullopt;
    }
    std::string_view command = message.substr(0, first_space);
    // Needs to be one of the known commands.
    if (!(command == "move" || command == "switch" || command == "team")) {
      return std::nullopt;
    }

    return BotCommand{std::string(message.substr(0, first_space)),
                      std::string(message.substr(first_space + 1))};
  }
};

using Message =
    std::variant<WebsocketMessage, CompoundWebsocketMessage, Team, BotCommand>;

// Enum for the different stages of the Showdown client.
enum class ShowdownClientStateEnum {
  kLoggingIn,
  kJoinLobby,
  kAcceptChallenge,
  kInBattle,
  kDisconnecting,
};

struct WebsocketState {
  using WriteCallback = std::function<void(const std::string&)>;
  WebsocketState(const WriteCallback& socket_callback,
                 const WriteCallback& fifo_callback)
      : socket_write{socket_callback}, fifo_write{fifo_callback} {}
  WebsocketState(const WebsocketState&) = default;
  WebsocketState(WebsocketState&&) = default;
  WebsocketState& operator=(const WebsocketState&) = default;
  WebsocketState& operator=(WebsocketState&&) = default;
  ~WebsocketState() = default;

  // Set the message header and contents.
  void SetMessage(std::string_view message) {
    std::optional<CompoundWebsocketMessage> compound_message_or =
        CompoundWebsocketMessage::CreateCompoundMessage(message);
    if (compound_message_or.has_value()) {
      std::cout << "Received compound message." << std::endl;
      last_message = compound_message_or.value();
      return;
    }
    std::optional<Team> team_or = Team::CreateTeam(message);
    if (team_or.has_value()) {
      std::cout << "Received team: " << team_or.value().team_as_str
                << std::endl;
      last_message = team_or.value();
      return;
    }
    std::optional<WebsocketMessage> message_or =
        WebsocketMessage::CreateMessage(message);
    if (message_or.has_value()) {
      last_message = message_or.value();
      return;
    }
    std::optional<BotCommand> command_or = BotCommand::CreateCommand(message);
    if (command_or.has_value()) {
      last_message = command_or.value();
      return;
    }
    std::cerr << "Unknown message type: " << message << std::endl;
  }

  // The last received message from the server.
  Message last_message;

  // Callback for writing messages to the server.
  WriteCallback socket_write;

  // Callback for writing messages to the FIFO.
  WriteCallback fifo_write;
};

using ShowdownClientStateMachine =
    state_machine::StateMachine<ShowdownClientStateEnum, WebsocketState>;

}  // namespace ps_client
