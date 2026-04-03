#include <openssl/ssl.h>
#include <browser/url.hpp>
#include <filesystem>
#include <format>

std::expected<Url, std::string> Url::parse_url(std::string_view url) {
    Url result = {};

    const size_t scheme_sep = url.find("://");

    if (scheme_sep == std::string_view::npos) {
        return std::unexpected("ERROR: No scheme found.");
    }

    result.scheme_ = std::string{url.substr(0, scheme_sep)};
    std::string_view rest = url.substr(scheme_sep + 3);

    if (result.scheme_ == "http") {
        result.port_ = HTTP_PORT;
    } else if (result.scheme_ == "https") {
        result.port_ = HTTPS_PORT;
    } else {
        return std::unexpected("ERROR: Only scheme http and https are supported.");
    }

    const size_t path_sep = rest.find('/');
    if (path_sep == std::string_view::npos) {
        result.host_ = std::string{rest};
        result.path_ = "/";
    } else {
        result.host_ = std::string{rest.substr(0, path_sep)};
        result.path_ = std::string{rest.substr(path_sep)};
    }

    const size_t port_sep = result.host_.find(':');
    if (port_sep != std::string::npos) {
        result.port_ = std::stoi(result.host_.substr(port_sep + 1));
        result.host_ = result.host_.substr(0, port_sep);
    }

    return result;
}

std::expected<std::string, std::string> Url::request() const {
    asio::io_context io_context;
    asio::ip::tcp::resolver resolver(io_context);
    asio::error_code ec;

    const asio::ip::tcp::resolver::results_type endpoints =
        resolver.resolve(host_, std::to_string(port_), ec);
    if (ec) {
        return std::unexpected(std::format("ERROR: DNS Error: {}.", ec.message()));
    }

    std::string request_text = std::format("GET {} HTTP/1.0\r\n", path_);
    request_text += std::format("Host: {}\r\n", host_);
    request_text += "User-Agent: toy-browser\r\n";
    request_text += "Connection: close\r\n\r\n";

    if (scheme_ == "https") {
        // Create a context that uses the default paths for finding CA certificates.
        asio::ssl::context ctx(asio::ssl::context::sslv23);

        ec = ctx.set_default_verify_paths(ec);
        if (ec) {
            return std::unexpected(std::format("ERROR: Connection failed: {}.", ec.message()));
        }

        // On Windows, default verify paths often trust no CAs; load Mozilla bundle from assets
        // when checked in. Also helps minimal Linux images if cacert.pem is present.
        namespace fs = std::filesystem;
        for (const char* rel : {"assets/cacert.pem", "cacert.pem"}) {
            std::error_code fs_ec;
            if (fs::exists(rel, fs_ec)) {
                asio::error_code load_ec;
                ctx.load_verify_file(rel, load_ec);
                if (!load_ec) break;
            }
        }

        // Open a socket and connect to remote host.
        asio::ssl::stream<asio::ip::tcp::socket> socket(io_context, ctx);
        asio::connect(socket.lowest_layer(), endpoints, ec);
        if (ec) {
            return std::unexpected(std::format("ERROR: Connection failed: {}.", ec.message()));
        }

        // Set SNI Hostname for modern HTTPS.
        SSL_set_tlsext_host_name(socket.native_handle(), host_.c_str());

        ec = socket.lowest_layer().set_option(asio::ip::tcp::no_delay(true), ec);
        if (ec) {
            return std::unexpected(
                std::format("ERROR: Setting SSL option failed: {}.", ec.message()));
        }

        // Perform SSL handshake and verify the remote host's certificate.
        ec = socket.set_verify_mode(asio::ssl::verify_peer, ec);
        if (ec) {
            return std::unexpected(
                std::format("ERROR: Could not verify peer SSL: {}.", ec.message()));
        }

        ec = socket.set_verify_callback(asio::ssl::host_name_verification(host_), ec);
        if (ec) {
            return std::unexpected(
                std::format("ERROR: Certificate did not pass pre-verification: {}.", ec.message()));
        }

        ec = socket.handshake(asio::ssl::stream<asio::ip::tcp::socket>::client, ec);
        if (ec) {
            return std::unexpected(std::format("ERROR: SSL Handshake failed: {}.", ec.message()));
        }

        return send_request(socket, request_text);
    } else {
        asio::ip::tcp::socket socket(io_context);
        asio::connect(socket, endpoints, ec);

        if (ec) {
            return std::unexpected(std::format("ERROR: Connection failed: {}.", ec.message()));
        }

        return send_request(socket, request_text);
    }
}

std::expected<Url, std::string> Url::resolve(std::string_view url) const {
    // Case 1: Absolute URL (e.g., "https://google.com/logo.png").
    if (url.contains("://")) {
        return parse_url(url);
    }

    std::string path_buffer{url};

    // Case 2: Path-relative URL (e.g., "styles.css" or "../images/icon.png")
    // If it doesn't start with "/", it's relative to the current directory.
    if (!path_buffer.starts_with("/")) {
        size_t last_slash = path_.rfind('/');
        std::string_view dir = (last_slash == std::string_view::npos)
                                   ? ""
                                   : std::string_view(path_).substr(0, last_slash);
        while (path_buffer.starts_with("../")) {
            path_buffer = path_buffer.substr(3);
            if (dir.contains('/')) {
                size_t prev_dir = dir.rfind('/');
                dir = dir.substr(0, prev_dir);
            } else {
                dir = "";  // We hit the root.
            }
        }
        path_buffer = std::string(dir) + "/" + path_buffer;
    }

    // Case 3: Scheme-relative URL (e.g., "//cdn.example.com/lib.js").
    if (path_buffer.starts_with("//")) {
        return parse_url(scheme_ + ":" + path_buffer);
    }
    // Case 4: Host-relative URL (e.g., "/top-level-style.css")
    // Keep scheme, host, and port, but replace the path entirely.
    else {
        std::string full_url = scheme_ + "://" + host_ + ":" + std::to_string(port_) + path_buffer;
        return parse_url(full_url);
    }
}
