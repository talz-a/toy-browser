#include <algorithm>
#include <asio.hpp>
#include <asio/read.hpp>
#include <cassert>
#include <cctype>
#include <format>
#include <iostream>
#include <map>
#include <print>
#include <ranges>
#include <string>
#include <string_view>

// TODO: Move this to own file once working.
class url {
public:
    explicit url(std::string_view url_string) {
        const auto scheme_sep = url_string.find("://");

        scheme_ = std::string{url_string.substr(0, scheme_sep)};
        const std::string_view rest = url_string.substr(scheme_sep + 3);

        assert("ERROR: Only scheme http is supported" && scheme_ == "http");

        const auto path_sep = rest.find('/');
        if (path_sep == std::string_view::npos) {
            host_ = std::string{rest};
            path_ = "/";
        } else {
            host_ = std::string{rest.substr(0, path_sep)};
            path_ = std::string{rest.substr(path_sep)};
        }
    }

    [[nodiscard]] std::string request() const {
        // TODO: Move all this connection logic out?
        asio::io_context io_context;

        asio::ip::tcp::resolver resolver(io_context);
        const auto endpoints = resolver.resolve(host_, "80");

        asio::ip::tcp::socket socket(io_context);
        asio::connect(socket, endpoints);

        std::string request = std::format("GET {} HTTP/1.0\r\n", path_);
        request += std::format("Host: {}\r\n", host_);
        request += "\r\n";
        auto buf = asio::buffer(request);
        asio::write(socket, buf);

        asio::streambuf response_buffer;
        asio::read_until(socket, response_buffer, "\r\n");

        std::istream response_stream(&response_buffer);
        std::string status_line;
        std::getline(response_stream, status_line);
        if (!status_line.empty() && status_line.back() == '\r') {
            status_line.pop_back();
        }

        // auto parts =
        //     status_line | std::views::split(' ') | std::ranges::to<std::vector<std::string>>();
        // std::string version = parts.at(0);
        // std::string status = parts.at(1);
        // std::string explanation = parts.at(2);

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

            const auto colon = line.find(':');
            std::string header = line.substr(0, colon);
            std::string value = line.substr(colon + 1);

            std::ranges::transform(
                header, header.begin(), [](unsigned char c) { return std::tolower(c); });

            const auto is_space = [](unsigned char c) { return std::isspace(c); };
            auto trimmed_value = value | std::views::drop_while(is_space) | std::views::reverse |
                                 std::views::drop_while(is_space) | std::views::reverse |
                                 std::ranges::to<std::string>();

            response_headers[header] = trimmed_value;
        }

        assert("ERROR: transfer-encoding missing" &&
               !response_headers.contains("transfer-encoding"));
        assert("ERROR: content-encoding" && !response_headers.contains("content-encoding"));

        asio::error_code ec;
        asio::read(socket, response_buffer, asio::transfer_all(), ec);
        if (ec && ec != asio::error::eof) {
            std::cerr << "Error reading: " << ec.message() << "\n";
        }
        socket.close();

        return std::string{asio::buffers_begin(response_buffer.data()),
                           asio::buffers_end(response_buffer.data())};
    }

private:
    std::string scheme_;
    std::string host_;
    std::string path_;
};

void show(std::string_view body) {
    bool in_tag = false;
    for (char c : body) {
        if (c == '<') {
            in_tag = true;
        } else if (c == '>') {
            in_tag = false;
        } else if (!in_tag) {
            std::print("{}", c);
        }
    }
}

void load(const url& target) {
    const std::string body = target.request();
    show(body);
}

int main(int argc, char* argv[]) {
    if (argc == 1) {
        std::cerr << "ERROR: No arguments given\n";
        return -1;
    }
    load(url(argv[1]));
}