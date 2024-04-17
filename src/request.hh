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

std::string_view format_method(Method method);
std::ostream& operator<<(std::ostream& os, const Method& method);

class Request {
    Method method_;
    std::string target_;
    Headers headers_;
    Params params_;
    std::string body_;

public:
    Request(const Method method, const std::string& target,
            const Headers& headers, const std::string& body)
        : method_{method}, target_{target}, headers_{headers}, body_{body} {}

    void set_params(Params params);

    std::string_view target() const;
    Method method() const;
    std::string_view body() const;
    std::optional<std::string_view> header(std::string_view key) const;
    std::optional<std::string_view> path_parameter(std::string_view key) const;
};
}  // namespace http
