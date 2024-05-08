#pragma once

#include <memory>
#include <optional>
#include <string_view>
#include <utility>

#include "types.hh"

namespace waxwing {
enum class HttpMethod {
    Get,
    Head,
    Post,
    Put,
    Delete,
    Connect,
    Options,
    Trace,
    Patch,
};

std::string_view format_method(HttpMethod method) noexcept;
std::optional<HttpMethod> parse_method(std::string_view s) noexcept;

class Request {
    friend class RequestBuilder;

    HttpMethod method_;
    std::string target_;
    Headers headers_;
    Params params_;
    std::string body_;

    Request(HttpMethod method, std::string&& target, Headers&& headers,
            std::string&& body);

public:
    Request(const Request&) = delete;
    Request& operator=(const Request&) = delete;

    Request(Request&& rhs) noexcept;
    Request& operator=(Request&& rhs) noexcept;

    std::string_view target() const noexcept;
    HttpMethod method() const noexcept;
    std::string_view body() const noexcept;
    std::optional<std::string_view> header(std::string_view key) const noexcept;
};

class RequestBuilder final {
    HttpMethod method_;
    std::string target_;
    Headers headers_{};
    std::string body_{};

public:
    RequestBuilder(const RequestBuilder&) = delete;
    RequestBuilder& operator=(const RequestBuilder&) = delete;

    template <typename S>
        requires(std::is_constructible_v<std::string, S>)
    RequestBuilder(HttpMethod method, S&& target)
        : method_{method}, target_{std::forward<S>(target)} {}

    template <typename S1, typename S2>
        requires(std::is_constructible_v<std::string, S1>) &&
                (std::is_constructible_v<std::string, S2>)
    void header(S1&& key, S2&& value) {
        headers_.emplace(std::forward<S1>(key), std::forward<S2>(value));
    }

    template <typename S>
        requires(std::is_constructible_v<std::string, S>)
    void body(S&& value) {
        std::construct_at(&body_, std::forward<S>(value));
    }

    std::unique_ptr<Request> build();
};
}  // namespace waxwing
