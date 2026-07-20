#pragma once

#include <cstdint>
#include <expected>
#include <string>
#include <string_view>
#include <utility>
#include <variant>

struct UrlParseError {
    std::string message;
};
struct UrlRequestError {
    std::string operation;
    std::string message;
};

using UrlError = std::variant<UrlParseError, UrlRequestError>;

[[nodiscard]] std::string url_error_message(const UrlError& error);

class Url {
public:
    [[nodiscard]] static std::expected<Url, UrlError> parse(std::string_view url);

    [[nodiscard]] std::expected<std::string, UrlError> request() const;

    [[nodiscard]] std::expected<Url, UrlError> resolve(std::string_view url) const;

private:
    enum class Scheme { http, https };

    static constexpr std::uint16_t HTTP_PORT = 80;
    static constexpr std::uint16_t HTTPS_PORT = 443;

    Url(Scheme scheme, std::string host, std::string path, std::uint16_t port)
        : scheme_(scheme), host_(std::move(host)), path_(std::move(path)), port_(port) {}

    [[nodiscard]] std::string_view scheme_name() const noexcept;

    Scheme scheme_;
    std::string host_;
    std::string path_;
    std::uint16_t port_;
};
