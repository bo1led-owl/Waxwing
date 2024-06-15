#pragma once

#include <optional>
#include <string_view>
#include <utility>

#include "waxwing/http.hh"

namespace waxwing {
class Request {
    friend class RequestBuilder;

    HttpMethod method_;
    std::string target_;
    Headers headers_;
    std::string body_;

    Request(HttpMethod method, std::string&& target, Headers&& headers,
            std::string&& body);

public:
    Request(const Request&) = delete;
    Request& operator=(const Request&) = delete;

    Request(Request&& rhs) noexcept = default;
    Request& operator=(Request&& rhs) noexcept = default;

    HttpMethod method() const noexcept;
    std::string_view target() const noexcept;
    std::string_view body() const noexcept;
    std::optional<std::string_view> header(std::string_view key) const noexcept;
};

class RequestBuilder final {
    HttpMethod method_;
    std::string target_;
    Headers headers_;
    std::string body_;

public:
    template <typename S>
        requires(std::constructible_from<std::string, S>)
    RequestBuilder(HttpMethod method, S&& target)
        : method_{method}, target_{std::forward<S>(target)} {}

    template <typename S1, typename S2>
        requires(std::constructible_from<std::string, S1>) &&
                (std::constructible_from<std::string, S2>)
    RequestBuilder& header(S1&& key, S2&& value) & {
        headers_[std::forward<S1>(key)] = std::forward<S2>(value);
        return *this;
    }

    template <typename S1, typename S2>
        requires(std::constructible_from<std::string, S1>) &&
                (std::constructible_from<std::string, S2>)
    RequestBuilder&& header(S1&& key, S2&& value) && {
        headers_[std::forward<S1>(key)] = std::forward<S2>(value);
        return std::move(*this);
    }

    RequestBuilder& headers(const Headers& headers) & {
        headers_ = headers;
        return *this;
    }

    RequestBuilder&& headers(Headers&& headers) && noexcept {
        headers_ = std::move(headers);
        return std::move(*this);
    }

    template <typename S>
        requires(std::constructible_from<std::string, S>)
    RequestBuilder& body(S&& value) & {
        body_ = std::forward<S>(value);
        return *this;
    }

    template <typename S>
        requires(std::constructible_from<std::string, S>)
    RequestBuilder&& body(S&& value) && {
        body_ = std::forward<S>(value);
        return std::move(*this);
    }

    Request build() &&;
};
}  // namespace waxwing
