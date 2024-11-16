#include <string>

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <nlohmann/json.hpp>

namespace ps_client {
class User {
public:
    struct UserInfo {
        std::string_view name;
        std::string_view pass;
        std::string_view challstr;
    };

    User(boost::asio::io_context& ioc) 
        : resolver_(ioc) {
        ssl_context_.set_default_verify_paths();
        ssl_context_.set_verify_mode(boost::asio::ssl::verify_peer);
        stream_ = std::make_unique<boost::beast::ssl_stream<boost::beast::tcp_stream>>(
            ioc, ssl_context_);
    }

    std::string Login(const UserInfo& user_info);
private:
    static constexpr std::string_view kHost{"play.pokemonshowdown.com"};
    static constexpr std::string_view kTarget{"/api/login"};
    static constexpr int kVersion = 11;

    boost::asio::ssl::context ssl_context_{boost::asio::ssl::context::sslv23_client};

    boost::asio::ip::tcp::resolver resolver_;
    std::unique_ptr<boost::beast::ssl_stream<boost::beast::tcp_stream>> stream_;
};

} // namespace ps_client
