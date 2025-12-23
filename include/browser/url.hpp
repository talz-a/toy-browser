#pragma once

#include <asio.hpp>
#include <asio/ssl.hpp>
#include <map>
#include <ranges>
#include <string>
#include <string_view>

class url {
public:
    explicit url(std::string_view url_string);

    [[nodiscard]] std::string request() const;

private:
    std::string scheme_;
    std::string host_;
    std::string path_;
    int port_;

    template <typename Stream>
    std::string send_request(Stream& stream, const std::string& request_text) const {
        asio::write(stream, asio::buffer(request_text));

        asio::streambuf response_buffer;
        asio::read_until(stream, response_buffer, "\r\n");

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
                std::string value = line.substr(colon + 1);

                std::ranges::transform(
                    header, header.begin(), [](unsigned char c) { return std::tolower(c); });

                auto is_space = [](unsigned char c) { return std::isspace(c); };
                auto trimmed_value = value | std::views::drop_while(is_space) |
                                     std::views::reverse | std::views::drop_while(is_space) |
                                     std::views::reverse | std::ranges::to<std::string>();

                response_headers[header] = trimmed_value;
            }
        }

        if (response_headers.contains("transfer-encoding")) {
            throw std::runtime_error("ERROR: Transfer-encoding not supported.");
        }
        if (response_headers.contains("content-encoding")) {
            throw std::runtime_error("ERROR: Content-encoding not supported.");
        }

        try {
            asio::read(stream, response_buffer, asio::transfer_all());
        } catch (const asio::system_error& e) {
            if (e.code() != asio::error::eof) {
                throw;
            }
        }

        return std::string{asio::buffers_begin(response_buffer.data()),
                           asio::buffers_end(response_buffer.data())};
    }
};