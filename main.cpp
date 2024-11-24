#include <boost/asio/connect.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include "accept_challenge_state.h"
#include "fifo_listener.h"
#include "in_battle_state.h"
#include "lobby_state.h"
#include "login_state.h"
#include "shared_queue.h"
#include "showdown_state_machine.h"

namespace beast = boost::beast;          // from <boost/beast.hpp>
namespace http = beast::http;            // from <boost/beast/http.hpp>
namespace websocket = beast::websocket;  // from <boost/beast/websocket.hpp>
namespace net = boost::asio;             // from <boost/asio.hpp>
using tcp = net::ip::tcp;                // from <boost/asio/ip/tcp.hpp>
constexpr beast::string_view kWebSocketPath = "/showdown/websocket";

class WebSocketClient {
 public:
  WebSocketClient(net::io_context& ioc, const std::string& host,
                  const std::string& port,
                  std::shared_ptr<util::SharedQueue<std::string>> message_queue)
      : strand_(net::make_strand(ioc)),
        resolver_(ioc),
        ws_(net::make_strand(ioc)),
        host_(host),
        message_queue_(message_queue) {
    // Resolve the hostname and port synchronously
    auto const results = resolver_.resolve(host, port);

    // Connect synchronously
    net::connect(ws_.next_layer(), results);

    // Perform the WebSocket handshake
    ws_.handshake(host_, kWebSocketPath);

    // Start reading messages asynchronously
    do_read();
  }

  void write(const std::string& message) {
    net::dispatch(strand_, [this, message]() {
      std::cout << "Writing message: " << message << std::endl;
      std::string prepended = "|";
      prepended += message;
      ws_.async_write(
          net::buffer(prepended),
          [this](beast::error_code ec, std::size_t bytes_transferred) {
            on_write(ec, bytes_transferred);
          });
    });
  }

  void close() {
    ws_.async_close(websocket::close_code::normal,
                    [this](beast::error_code ec) { on_close(ec); });
  }

 private:
  void do_read() {
    ws_.async_read(buffer_,
                   [this](beast::error_code ec, std::size_t bytes_transferred) {
                     on_read(ec, bytes_transferred);
                   });
  }

  void on_read(beast::error_code ec, std::size_t bytes_transferred) {
    boost::ignore_unused(bytes_transferred);

    if (ec) {
      fail(ec, "read");
      return;
    }

    // Get the message from the buffer.
    std::string message{beast::buffers_to_string(buffer_.data())};
    message_queue_->Enqueue(message);

    // Clear the buffer
    buffer_.consume(buffer_.size());

    // Continue reading messages
    do_read();
  }

  void on_write(beast::error_code ec, std::size_t bytes_transferred) {
    boost::ignore_unused(bytes_transferred);

    if (ec) {
      fail(ec, "write");
    }
  }

  void on_close(beast::error_code ec) {
    if (ec) {
      fail(ec, "close");
    }
  }

  void fail(beast::error_code ec, const char* what) {
    std::cerr << what << ": " << ec.message() << "\n";
  }

  net::strand<net::io_context::executor_type> strand_;
  tcp::resolver resolver_;
  websocket::stream<tcp::socket> ws_;
  beast::flat_buffer buffer_;
  std::string host_;
  std::shared_ptr<util::SharedQueue<std::string>> message_queue_;
};

// Class that takes Message objects from a queue and calls the StateMachine
// update.
// TODO: Use smart pointers / move semantics / factory.
class MessageHandler {
 public:
  explicit MessageHandler(
      ps_client::ShowdownClientStateMachine* state_machine,
      std::shared_ptr<util::SharedQueue<std::string>> message_queue)
      : state_machine_(state_machine), message_queue_(message_queue) {}

  void HandleMessage(const std::string& message) {
    state_machine_->MutableContext()->SetMessage(message);
    state_machine_->Update();
  }

  // Runs a loop that reads messages from the queue and calls HandleMessage.
  void Run() {
    while (true) {
      std::optional<std::string> message = message_queue_->Dequeue();
      if (!message.has_value()) {
        break;
      }
      std::cout << "Received message: " << message.value() << std::endl;
      HandleMessage(message.value());
    }
  }

 private:
  ps_client::ShowdownClientStateMachine* state_machine_;
  std::shared_ptr<util::SharedQueue<std::string>> message_queue_;
};

int main(int argc, char** argv) {
  if (argc != 3) {
    std::cerr << "Usage: " << argv[0] << " <host> <port>\n";
    return EXIT_FAILURE;
  }
  const std::string host = argv[1];
  const std::string port = argv[2];

  auto shared_message_queue =
      std::make_shared<util::SharedQueue<std::string>>();

  net::io_context ioc;
  WebSocketClient client(ioc, host, port, shared_message_queue);

  // Run the I/O context on a separate thread
  std::thread t([&ioc] { ioc.run(); });

  // Start the FIFO listener
  std::thread fifo_listener(fifo::ReadFromFIFO, "/tmp/ps_fifo",
                            shared_message_queue);

  // Create the FIFO writer.
  fifo::FIFOWriter fifo_writer("/tmp/fifo_to_bot");

  // Create the state machine
  ps_client::ShowdownClientStateMachine::ContextType context{
      [&client](const std::string& message) { client.write(message); },
      fifo_writer.GetWriteFn()};
  ps_client::ShowdownClientStateMachine state_machine(&context);
  // Add the states to the state machine.
  state_machine.AddState(ps_client::ShowdownClientStateEnum::kLoggingIn,
                         std::make_unique<ps_client::LoginState>(ioc));
  state_machine.AddState(ps_client::ShowdownClientStateEnum::kJoinLobby,
                         std::make_unique<ps_client::LobbyState>());
  state_machine.AddState(ps_client::ShowdownClientStateEnum::kAcceptChallenge,
                         std::make_unique<ps_client::AcceptChallengeState>());
  state_machine.AddState(ps_client::ShowdownClientStateEnum::kInBattle,
                         std::make_unique<ps_client::InBattleState>());
  state_machine.Start(ps_client::ShowdownClientStateEnum::kLoggingIn);
  MessageHandler handler(&state_machine, shared_message_queue);
  handler.Run();

  // // Keep reading input and sending messages to the WebSocket
  // std::string line;
  // while(std::getline(std::cin, line)) {
  //     client.write(line);
  // }

  t.join();
  client.close();

  return EXIT_SUCCESS;
}
