#pragma once

#include <optional>
#include <string_view>

#include "types.hh"

namespace http {
enum class Method {
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

std::string_view format_method(Method method) noexcept;
std::optional<Method> parse_method(std::string_view s) noexcept;

class Request {
    Method method_;
    std::string target_;
    internal::Headers headers_;
    internal::Params params_;
    std::string body_;

public:
    template <typename T, typename U>
        requires(std::is_constructible_v<std::string, T>) && (std::is_constructible_v<std::string, U>)
    Request(Method method, T&& target,
            const internal::Headers& headers, U&& body)
        : method_{method}, target_{std::forward<T>(target)}, headers_{headers}, body_{std::forward<U>(body)} {}

    template <typename T, typename U>
        requires(std::is_constructible_v<std::string, T>) && (std::is_constructible_v<std::string, U>)
    Request(Method method, T&& target,
            internal::Headers&& headers, U&& body)
        : method_{method}, target_{std::forward<T>(target)}, headers_{std::move(headers)}, body_{std::forward<U>(body)} {}

    void set_params(const internal::Params& params) noexcept;
    void set_params(internal::Params&& params) noexcept;

    std::string_view target() const noexcept;
    Method method() const noexcept;
    std::string_view body() const noexcept;
    std::optional<std::string_view> header(std::string_view key) const noexcept;
    std::string_view path_parameter(std::string_view key) const noexcept;
};
}  // namespace http
