#include "request.hh"

#include <cassert>
#include <utility>

#include "str_util.hh"

namespace http {
std::string_view format_method(const Method method) {
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
    }

    __builtin_unreachable();
}

void Request::set_params(internal::Params params) {
    params_ = std::move(params);
}

std::string_view Request::body() const {
    return body_;
}

std::string_view Request::target() const {
    return target_;
}

Method Request::method() const {
    return method_;
}

std::optional<std::string_view> Request::header(
    const std::string_view key) const {
    const internal::Headers::const_iterator result =
        headers_.find(str_util::to_lower(key));
    if (result == headers_.end()) {
        return std::nullopt;
    }

    return result->second;
}

std::string_view Request::path_parameter(const std::string_view key) const {
    internal::Params::const_iterator result =
        params_.find(str_util::to_lower(key));

    assert(
        result !=
        headers_.end());  // fail here means that router is working incorrectly

    return result->second;
}
}  // namespace http
