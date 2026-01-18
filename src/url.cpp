#include "browser/url.hpp"
#include <openssl/ssl.h>
#include <format>
#include <stdexcept>

url::url(std::string_view url_string) {
    const auto scheme_sep = url_string.find("://");
    if (scheme_sep == std::string_view::npos) throw std::runtime_error("ERROR: No scheme found.");

    scheme_ = std::string{url_string.substr(0, scheme_sep)};
    const std::string_view rest = url_string.substr(scheme_sep + 3);

    if (scheme_ == "http") {
        port_ = http_port_;
    } else if (scheme_ == "https") {
        port_ = https_port_;
    } else {
        throw std::runtime_error("ERROR: Only scheme http and https are supported.");
    }

    const auto path_sep = rest.find('/');
    if (path_sep == std::string_view::npos) {
        host_ = std::string{rest};
        path_ = "/";
    } else {
        host_ = std::string{rest.substr(0, path_sep)};
        path_ = std::string{rest.substr(path_sep)};
    }

    if (const auto port_sep = host_.find(':'); port_sep != std::string::npos) {
        port_ = std::stoi(host_.substr(port_sep + 1));
        host_ = host_.substr(0, port_sep);
    }
}

std::string url::request() const {
    asio::io_context io_context;

    asio::ip::tcp::resolver resolver(io_context);
    const auto endpoints = resolver.resolve(host_, std::to_string(port_));

    std::string request_text = std::format("GET {} HTTP/1.0\r\n", path_);
    request_text += std::format("Host: {}\r\n", host_);
    request_text += "User-Agent: toy-browser\r\n";
    request_text += "Connection: close\r\n\r\n";

    if (scheme_ == "https") {
        asio::ssl::context ctx(asio::ssl::context::sslv23);
        ctx.set_default_verify_paths();
        asio::ssl::stream<asio::ip::tcp::socket> stream(io_context, ctx);

        if (!SSL_set_tlsext_host_name(stream.native_handle(), host_.c_str())) {
            throw std::runtime_error("ERROR: SSL setup failed");
        }

        asio::connect(stream.lowest_layer(), endpoints);
        stream.handshake(asio::ssl::stream_base::client);
        return send_request(stream, request_text);
    }

    asio::ip::tcp::socket socket(io_context);
    asio::connect(socket, endpoints);
    return send_request(socket, request_text);
}
