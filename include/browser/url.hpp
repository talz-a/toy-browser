#pragma once

#include <asio.hpp>
#include <asio/ssl.hpp>
#include <expected>
#include <map>

#include <string_view>

enum class browser_error : std::uint8_t {
    network_error,
    unsupported_transfer_encoding,
    unsupported_content_encoding,
    invalid_response,
    ssl_error
};

class url {
public:
    explicit url(std::string_view url_string);

    [[nodiscard]] std::expected<std::string, browser_error> request() const;

private:
    std::string scheme_;
    std::string host_;
    std::string path_;
    int port_;

    static constexpr unsigned int HTTP_PORT = 80;
    static constexpr unsigned int HTTPS_PORT = 443;

    template <typename Stream>
    std::expected<std::string, browser_error> send_request(Stream& stream,
                                                           const std::string& request_text) const {
        asio::error_code ec;

        asio::write(stream, asio::buffer(request_text), ec);
        if (ec) {
            return std::unexpected(browser_error::network_error);
        }

        asio::streambuf response_buffer;
        asio::read_until(stream, response_buffer, "\r\n", ec);
        if (ec) {
            return std::unexpected(browser_error::network_error);
        }

        std::istream response_stream(&response_buffer);
        std::string status_line;
        std::getline(response_stream, status_line);
        if (!status_line.empty() && status_line.back() == '\r') {
            status_line.pop_back();
        }

        std::map<std::string, std::string> response_headers;
        while (true) {
            std::string line;
            std::getline(response_stream, line);
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }
            if (line.empty()) {
                break;
            }

            if (const auto colon = line.find(':'); colon != std::string::npos) {
                std::string header = line.substr(0, colon);
                std::string raw_value = line.substr(colon + 1);

                std::ranges::transform(
                    header, header.begin(), [](unsigned char c) { return std::tolower(c); });

                std::string_view sv = raw_value;

                while (!sv.empty() && std::isspace(static_cast<unsigned char>(sv.front()))) {
                    sv.remove_prefix(1);
                }

                while (!sv.empty() && std::isspace(static_cast<unsigned char>(sv.back()))) {
                    sv.remove_suffix(1);
                }

                response_headers[header] = std::string(sv);
            }
        }

        if (response_headers.contains("transfer-encoding")) {
            return std::unexpected(browser_error::unsupported_transfer_encoding);
        }
        if (response_headers.contains("content-encoding")) {
            return std::unexpected(browser_error::unsupported_content_encoding);
        }

        asio::read(stream, response_buffer, asio::transfer_all(), ec);

        if (ec && ec != asio::error::eof) {
            return std::unexpected(browser_error::network_error);
        }

        return std::string{asio::buffers_begin(response_buffer.data()),
                           asio::buffers_end(response_buffer.data())};
    }
};