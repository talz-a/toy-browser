#pragma once

#include <browser/utils.hpp>
#include <asio.hpp>
#include <asio/ssl.hpp>
#include <string_view>
#include <expected>

static constexpr uint16_t HTTP_PORT = 80;
static constexpr uint16_t HTTPS_PORT = 443;

struct Url {
    [[nodiscard]] static std::expected<Url, std::string> parse_url(std::string_view url);

    [[nodiscard]] std::expected<std::string, std::string> request() const;

    [[nodiscard]] std::expected<Url, std::string> resolve(std::string_view url) const;

    template <typename Stream>
    std::expected<std::string, std::string> send_request(Stream& stream, std::string_view request_text) const {
        asio::error_code ec;

        asio::write(stream, asio::buffer(request_text), ec);
        if (ec) {
            return std::unexpected(std::format("ERROR: Write failed: {}.", ec.message()));
        }

        asio::streambuf response_buffer;
        asio::read_until(stream, response_buffer, "\r\n\r\n", ec);
        if (ec) {
            return std::unexpected(std::format("ERROR: Header read failed: {}.", ec.message()));
        }

        std::istream response_stream(&response_buffer);
        std::string status_line;
        if (!std::getline(response_stream, status_line)) {
            return std::unexpected("ERROR: Empty response from server.");
        }

        if (!status_line.empty() && status_line.back() == '\r') status_line.pop_back();

        std::unordered_map<std::string, std::string> response_headers;
        std::string line;
        while (std::getline(response_stream, line) && line != "\r" && !line.empty()) {
            if (line.back() == '\r') line.pop_back();

            const size_t colon_pos = line.find(':');
            if (colon_pos != std::string::npos) {
                std::string header = to_lower(line.substr(0, colon_pos));
                std::string value = line.substr(colon_pos + 1);

                // Trim whitespace.
                value.erase(0, value.find_first_not_of(" \t"));
                auto last = value.find_last_not_of(" \t");
                if (last != std::string::npos) value.erase(last + 1);

                response_headers[header] = value;
            }
        }

        if (response_headers.contains("transfer-encoding")) {
            return std::unexpected("ERROR: Transfer-Encoding (chunked) is not supported.");
        }

        if (response_headers.contains("content-encoding")) {
            return std::unexpected("ERROR: Content-Encoding (compression) is not supported.");
        }

        asio::read(stream, response_buffer, asio::transfer_all(), ec);

        if (ec && ec != asio::error::eof && ec != asio::ssl::error::stream_truncated) {
            return std::unexpected(std::format("ERROR: Body read failed: {}.", ec.message()));
        }

        return std::string{
            asio::buffers_begin(response_buffer.data()), 
            asio::buffers_end(response_buffer.data())
        };
    }

    std::string scheme_;
    std::string host_;
    std::string path_;
    uint16_t port_;
};
