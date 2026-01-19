#pragma once

#include <asio.hpp>
#include <asio/ssl.hpp>
#include <string_view>
#include "browser/utils.hpp"

class url {
public:
    explicit url(std::string_view url_string);

    [[nodiscard]] std::string request() const;

private:
    std::string scheme_;
    std::string host_;
    std::string path_;
    int port_;

    static constexpr unsigned int http_port_ = 80;
    static constexpr unsigned int https_port_ = 443;

    template <typename Stream>
    std::string send_request(Stream& stream, const std::string& request_text) const {
        asio::write(stream, asio::buffer(request_text));

        asio::streambuf response_buffer;
        asio::read_until(stream, response_buffer, "\r\n");

        std::istream response_stream(&response_buffer);
        std::string status_line;
        std::getline(response_stream, status_line);
        if (!status_line.empty() && status_line.back() == '\r') status_line.pop_back();

        std::unordered_map<std::string, std::string> response_headers;
        while (true) {
            std::string line;
            std::getline(response_stream, line);

            if (!line.empty() && line.back() == '\r') line.pop_back();
            if (line.empty()) break;

            if (const auto colon = line.find(':'); colon != std::string::npos) {
                std::string header = to_lower(line.substr(0, colon));
                std::string raw_value = line.substr(colon + 1);

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
            throw std::runtime_error("ERROR: Transfer-Encoding is not supported.");
        }

        if (response_headers.contains("content-encoding")) {
            throw std::runtime_error("ERROR: Content-Encoding is not supported.");
        }

        asio::error_code ec;
        asio::read(stream, response_buffer, asio::transfer_all(), ec);

        if (ec && ec != asio::error::eof) throw std::system_error(ec);

        return std::string{
            asio::buffers_begin(response_buffer.data()), asio::buffers_end(response_buffer.data())
        };
    }
};
