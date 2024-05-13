#pragma once

#include <memory>
#include <optional>
#include <string>

#include "http.hh"
#include "types.hh"

namespace waxwing {
class Response final {
    friend class ResponseBuilder;

private:
    HttpStatusCode status_code_;
    Headers headers_;
    std::optional<std::string> body_;

    Response(HttpStatusCode code, Headers&& headers,
             std::optional<std::string>&& body) noexcept
        : status_code_{code},
          headers_{std::move(headers)},
          body_{std::move(body)} {}

public:
    Response(const Response&) = delete;
    Response& operator=(const Response&) = delete;

    Response(Response&& rhs) noexcept = default;
    Response& operator=(Response&&) noexcept = default;

    HttpStatusCode status() const noexcept;
    Headers& headers() & noexcept;
    std::optional<std::string>& body() & noexcept;
};

class ResponseBuilder final {
    HttpStatusCode status_code_;
    Headers headers_{};
    std::optional<std::string> body_{};

public:
    ResponseBuilder(HttpStatusCode code) noexcept : status_code_{code} {}

    template <typename S1, typename S2>
        requires(std::is_constructible_v<std::string, S1>) &&
                (std::is_constructible_v<std::string, S2>)
    ResponseBuilder& header(S1&& key, S2&& value) & {
        auto [iter, inserted] = headers_.try_emplace(std::forward<S1>(key),
                                                     std::forward<S2>(value));
        if (!inserted) {
            std::construct_at(&iter->second, std::forward<S2>(value));
        }
        return *this;
    }

    template <typename S1, typename S2>
        requires(std::is_constructible_v<std::string, S1>) &&
                (std::is_constructible_v<std::string, S2>)
    ResponseBuilder&& header(S1&& key, S2&& value) && {
        auto [iter, inserted] = headers_.try_emplace(std::forward<S1>(key),
                                                     std::forward<S2>(value));
        if (!inserted) {
            std::construct_at(&iter->second, std::forward<S2>(value));
        }
        return std::move(*this);
    }

    template <typename S>
        requires(std::is_constructible_v<std::string, S>)
    ResponseBuilder& body(S&& data) & {
        std::construct_at(&body_, std::forward<S>(data));
        return *this;
    }

    template <typename S>
        requires(std::is_constructible_v<std::string, S>)
    ResponseBuilder&& body(S&& data) && {
        std::construct_at(&body_, std::forward<S>(data));
        return std::move(*this);
    }

    template <typename S>
        requires(std::is_constructible_v<std::string, S>)
    ResponseBuilder& content_type(S&& type) & {
        return header("Content-Type", std::forward<S>(type));
    }

    template <typename S>
        requires(std::is_constructible_v<std::string, S>)
    ResponseBuilder&& content_type(S&& type) && {
        return std::move(
            std::move(*this).header("Content-Type", std::forward<S>(type)));
    }

    std::unique_ptr<Response> build() &&;
};
}  // namespace waxwing
