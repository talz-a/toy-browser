#include <openssl/ssl.h>
#include <asio.hpp>
#include <asio/ssl.hpp>
#include <browser/url.hpp>
#include <browser/utils.hpp>
#include <charconv>
#include <filesystem>
#include <format>
#include <istream>
#include <limits>
#include <system_error>
#include <unordered_map>

std::string url_error_message(const UrlError& error) {
    return std::visit(match{
                          [](const UrlParseError& error) {
                              return std::format("Invalid URL: {}", error.message);
                          },
                          [](const UrlRequestError& error) {
                              return std::format("{}: {}.", error.operation, error.message);
                          },
                      },
                      error);
}

namespace {
template <typename Stream>
std::expected<std::string, UrlError> send_request(Stream& stream, std::string_view request_text) {
    asio::write(stream, asio::buffer(request_text));

    asio::streambuf response_buffer;
    asio::read_until(stream, response_buffer, "\r\n\r\n");

    std::istream response_stream(&response_buffer);
    std::string status_line;
    if (!std::getline(response_stream, status_line)) {
        return std::unexpected(UrlRequestError{"Response failed", "Empty response from server"});
    }

    std::unordered_map<std::string, std::string> response_headers;
    std::string line;
    while (std::getline(response_stream, line) && line != "\r" && !line.empty()) {
        if (line.back() == '\r') line.pop_back();

        const std::size_t colon_pos = line.find(':');
        if (colon_pos != std::string::npos) {
            std::string header = to_lower(line.substr(0, colon_pos));
            std::string value = line.substr(colon_pos + 1);

            value.erase(0, value.find_first_not_of(" \t"));
            const auto last = value.find_last_not_of(" \t");
            if (last != std::string::npos) value.erase(last + 1);

            response_headers[header] = value;
        }
    }

    if (response_headers.contains("transfer-encoding")) {
        return std::unexpected(
            UrlRequestError{"Response failed", "Transfer-Encoding is not supported"});
    }

    if (response_headers.contains("content-encoding")) {
        return std::unexpected(
            UrlRequestError{"Response failed", "Content-Encoding is not supported"});
    }

    // An unknown-length HTTP/1.0 body ends at EOF, so EOF is successful completion here.
    asio::error_code ec;
    asio::read(stream, response_buffer, asio::transfer_all(), ec);
    if (ec && ec != asio::error::eof) {
        return std::unexpected(UrlRequestError{"Body read failed", ec.message()});
    }

    return std::string{asio::buffers_begin(response_buffer.data()),
                       asio::buffers_end(response_buffer.data())};
}
}  // namespace

std::expected<Url, UrlError> Url::parse(std::string_view url) {
    const std::size_t scheme_sep = url.find("://");

    if (scheme_sep == std::string_view::npos) {
        return std::unexpected(UrlParseError{"No scheme found."});
    }

    const std::string_view scheme_text = url.substr(0, scheme_sep);
    const std::string_view rest = url.substr(scheme_sep + 3);

    Scheme scheme;
    std::uint16_t port;
    if (scheme_text == "http") {
        scheme = Scheme::http;
        port = HTTP_PORT;
    } else if (scheme_text == "https") {
        scheme = Scheme::https;
        port = HTTPS_PORT;
    } else {
        return std::unexpected(UrlParseError{std::format(
            "Unsupported scheme '{}'; only HTTP and HTTPS are supported.", scheme_text)});
    }

    std::string host;
    std::string path;
    const std::size_t path_sep = rest.find('/');
    if (path_sep == std::string_view::npos) {
        host = rest;
        path = "/";
    } else {
        host = rest.substr(0, path_sep);
        path = rest.substr(path_sep);
    }

    const std::size_t port_sep = host.find(':');
    if (port_sep != std::string::npos) {
        const std::string_view port_text{host.data() + port_sep + 1, host.size() - port_sep - 1};
        unsigned int parsed_port = 0;
        const auto [end, error] =
            std::from_chars(port_text.data(), port_text.data() + port_text.size(), parsed_port);
        if (error != std::errc{} || end != port_text.data() + port_text.size() ||
            parsed_port == 0 || parsed_port > (std::numeric_limits<std::uint16_t>::max)()) {
            return std::unexpected(UrlParseError{std::format("Invalid port '{}'.", port_text)});
        }

        port = static_cast<std::uint16_t>(parsed_port);
        host.resize(port_sep);
    }

    if (host.empty()) {
        return std::unexpected(UrlParseError{"No host found."});
    }

    return Url{scheme, std::move(host), std::move(path), port};
}

std::string_view Url::scheme_name() const noexcept {
    switch (scheme_) {
        case Scheme::http:
            return "http";
        case Scheme::https:
            return "https";
    }

    std::unreachable();
}

std::expected<std::string, UrlError> Url::request() const {
    try {
        asio::io_context io_context;
        asio::ip::tcp::resolver resolver(io_context);
        const auto endpoints = resolver.resolve(host_, std::to_string(port_));

        std::string request_text = std::format("GET {} HTTP/1.0\r\n", path_);
        request_text += std::format("Host: {}\r\n", host_);
        request_text += "User-Agent: toy-browser\r\n";
        request_text += "Connection: close\r\n\r\n";

        if (scheme_ == Scheme::https) {
            asio::ssl::context ctx(asio::ssl::context::sslv23);
            ctx.set_default_verify_paths();

            // On Windows, default verify paths often trust no CAs; load Mozilla bundle from assets
            // when checked in. Also helps minimal Linux images if cacert.pem is present.
            namespace fs = std::filesystem;
            for (const char* rel : {"assets/cacert.pem", "cacert.pem"}) {
                std::error_code fs_ec;
                if (fs::exists(rel, fs_ec)) {
                    asio::error_code load_ec;
                    load_ec = ctx.load_verify_file(rel, load_ec);
                    if (!load_ec) break;
                }
            }

            asio::ssl::stream<asio::ip::tcp::socket> socket(io_context, ctx);
            asio::connect(socket.lowest_layer(), endpoints);

            SSL_set_tlsext_host_name(socket.native_handle(), host_.c_str());

            socket.lowest_layer().set_option(asio::ip::tcp::no_delay(true));

            socket.set_verify_mode(asio::ssl::verify_peer);
            socket.set_verify_callback(asio::ssl::host_name_verification(host_));
            socket.handshake(asio::ssl::stream<asio::ip::tcp::socket>::client);

            return send_request(socket, request_text);
        } else {
            asio::ip::tcp::socket socket(io_context);
            asio::connect(socket, endpoints);

            return send_request(socket, request_text);
        }
    } catch (const std::system_error& error) {
        return std::unexpected(UrlRequestError{"Request failed", error.what()});
    }
}

std::expected<Url, UrlError> Url::resolve(std::string_view url) const {
    // Case 1: Absolute URL (e.g., "https://google.com/logo.png").
    if (url.contains("://")) {
        return parse(url);
    }

    std::string path_buffer{url};

    // Case 2: Path-relative URL (e.g., "styles.css" or "../images/icon.png")
    // If it doesn't start with "/", it's relative to the current directory.
    if (!path_buffer.starts_with("/")) {
        const std::size_t last_slash = path_.rfind('/');
        std::string_view dir = (last_slash == std::string_view::npos)
                                   ? ""
                                   : std::string_view(path_).substr(0, last_slash);
        while (path_buffer.starts_with("../")) {
            path_buffer = path_buffer.substr(3);
            if (dir.contains('/')) {
                const std::size_t prev_dir = dir.rfind('/');
                dir = dir.substr(0, prev_dir);
            } else {
                dir = "";  // We hit the root.
            }
        }
        path_buffer = std::string(dir) + "/" + path_buffer;
    }

    // Case 3: Scheme-relative URL (e.g., "//cdn.example.com/lib.js").
    if (path_buffer.starts_with("//")) {
        return parse(std::string{scheme_name()} + ":" + path_buffer);
    }
    // Case 4: Host-relative URL (e.g., "/top-level-style.css")
    // Keep scheme, host, and port, but replace the path entirely.
    else {
        std::string full_url =
            std::format("{}://{}:{}{}", scheme_name(), host_, port_, path_buffer);
        return parse(full_url);
    }
}