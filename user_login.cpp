#include "user_login.h"

#include <iostream>

#include <boost/asio.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>

namespace ps_client {
namespace beast = boost::beast;     // from <boost/beast.hpp>
namespace http = beast::http;       // from <boost/beast/http.hpp>
namespace net = boost::asio;        // from <boost/asio.hpp>
using tcp = net::ip::tcp;           // from <boost/asio/ip/tcp.hpp>
using json = nlohmann::json;

// TODO: Make this function return an optional string.
std::string User::Login(const UserInfo& user_info) {
    // Resolve the host and connect
    auto const results = resolver_.resolve(kHost.to_string(), "https");
    // Connect to the host
    net::connect(beast::get_lowest_layer(*stream_).socket(), results.begin(), results.end());

    // Set SNI Hostname (many hosts need this to handshake successfully)
    if(!SSL_set_tlsext_host_name(stream_->native_handle(), kHost.data()))
    {
        throw boost::system::system_error(
            ::ERR_get_error(), boost::asio::error::get_ssl_category());
    }

    // Perform the SSL handshake
    stream_->handshake(net::ssl::stream_base::client);

    // Prepare the request body
    json data;
    data["name"] = user_info.name;  // replace with your bot's username
    data["pass"] = user_info.pass;  
    data["challstr"] = user_info.challstr;

    // Serialize JSON data to a string
    std::string body = data.dump();

    // Set up an HTTP POST request message
    http::request<http::string_body> req;
    req.method(http::verb::post);
    req.target(kTarget);
    req.version(kVersion);
    req.set(http::field::host, kHost);
    req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
    req.set(http::field::content_type, "application/x-www-form-urlencoded");
    req.content_length(body.size());
    req.body() = body;

    // Send the request
    http::write(*stream_, req);

    // Set up a buffer to receive the response
    beast::flat_buffer buffer;
    http::response<http::string_body> res;

    // Receive the response
    http::read(*stream_, buffer, res);

    // Parse the JSON response, skipping the first character
    std::cout << "HTTP response: " << res.body() << std::endl;
    json result_json = json::parse(res.body().substr(1));
    std::string assertion = result_json["assertion"];

    // Gracefully close the stream
    beast::error_code ec;
    stream_->shutdown(ec);
    if(ec == boost::asio::error::eof) {
        // Rationale:
        // http://www.boost.org/doc/libs/1_66_0/doc/html/boost_asio/reference/ssl__stream/async_shutdown/overload1.html
        ec.assign(0, ec.category());
    } else if (ec == boost::asio::ssl::error::stream_truncated) {
        std::cout << "Warning: stream truncated" << std::endl;
        ec.assign(0, ec.category());
    } else if(ec) {
        throw boost::system::system_error{ec};
    }

    // Return the assertion token
    return assertion;
}
} // namespace ps_client
