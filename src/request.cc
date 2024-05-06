#include "server/request.hh"

#include <cassert>
#include <utility>

#include "str_util.hh"

namespace http {
using namespace internal;

std::string_view format_method(const Method method) noexcept {
    switch (method) {
        case Method::Get:
            return "GET";
        case Method::Post:
            return "POST";
        case Method::Head:
            return "HEAD";
        case Method::Put:
            return "PUT";
        case Method::Delete:
            return "DELETE";
        case Method::Connect:
            return "CONNECT";
        case Method::Options:
            return "OPTIONS";
        case Method::Trace:
            return "TRACE";
        case Method::Patch:
            return "PATCH";
        default:
            __builtin_unreachable();
    }
}

std::optional<Method> parse_method(const std::string_view s) noexcept {
    if (s == "GET") {
        return Method::Get;
    }
    if (s == "POST") {
        return Method::Post;
    }
    if (s == "HEAD") {
        return Method::Head;
    }
    if (s == "PUT") {
        return Method::Put;
    }
    if (s == "DELETE") {
        return Method::Delete;
    }
    if (s == "CONNECT") {
        return Method::Connect;
    }
    if (s == "OPTIONS") {
        return Method::Options;
    }
    if (s == "TRACE") {
        return Method::Trace;
    }
    if (s == "PATCH") {
        return Method::Patch;
    }
    return std::nullopt;
}

void Request::set_params(const internal::Params& params) noexcept {
    params_ = params;
}

void Request::set_params(internal::Params&& params) noexcept {
    params_ = std::move(params);
}

std::string_view Request::body() const noexcept {
    return body_;
}

std::string_view Request::target() const noexcept {
    return target_;
}

Method Request::method() const noexcept {
    return method_;
}

std::optional<std::string_view> Request::header(
    const std::string_view key) const noexcept {
    const Headers::const_iterator result =
        headers_.find(str_util::to_lower(key));
    if (result == headers_.end()) {
        return std::nullopt;
    }

    return result->second;
}

std::string_view Request::path_parameter(
    const std::string_view key) const noexcept {
    Params::const_iterator result = params_.find(str_util::to_lower(key));

    assert(result != headers_.end() &&
           "Path parameter wasn't found");  // fail here means that router is
                                            // working incorrectly

    return result->second;
}
}  // namespace http
