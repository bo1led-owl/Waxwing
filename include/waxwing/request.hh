#pragma once

#include <optional>
#include <string_view>
#include <utility>

#include "types.hh"

namespace waxwing {
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
    Request(const Request&) = delete;
    Request& operator=(const Request&) = delete;

    Request(Request&& rhs) noexcept
        : method_{std::move(rhs.method_)},
          target_{std::move(rhs.target_)},
          headers_{std::move(rhs.headers_)},
          params_{std::move(rhs.params_)},
          body_{std::move(rhs.body_)} {}

    Request& operator=(Request&& rhs) noexcept {
        std::swap(method_, rhs.method_);
        std::swap(target_, rhs.target_);
        std::swap(headers_, rhs.headers_);
        std::swap(params_, rhs.params_);
        std::swap(body_, rhs.body_);
        return *this;
    }

    template <typename S1, typename S2>
        requires(std::is_constructible_v<std::string, S1>) &&
                    (std::is_constructible_v<std::string, S2>)
    Request(Method method, S1&& target, const internal::Headers& headers,
            S2&& body)
        : method_{method},
          target_{std::forward<S1>(target)},
          headers_{headers},
          body_{std::forward<S2>(body)} {}

    template <typename S1, typename S2>
        requires(std::is_constructible_v<std::string, S1>) &&
                    (std::is_constructible_v<std::string, S2>)
    Request(Method method, S1&& target, internal::Headers&& headers, S2&& body)
        : method_{method},
          target_{std::forward<S1>(target)},
          headers_{std::move(headers)},
          body_{std::forward<S2>(body)} {}

    void set_params(const internal::Params& params) noexcept;
    void set_params(internal::Params&& params) noexcept;

    std::string_view target() const noexcept;
    Method method() const noexcept;
    std::string_view body() const noexcept;
    std::optional<std::string_view> header(std::string_view key) const noexcept;
    std::string_view path_parameter(std::string_view key) const noexcept;
};
}  // namespace waxwing
