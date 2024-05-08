#include "waxwing/request.hh"

#include "str_util.hh"

namespace waxwing {
using namespace internal;

std::string_view format_method(const HttpMethod method) noexcept {
    switch (method) {
        case HttpMethod::Get:
            return "GET";
        case HttpMethod::Post:
            return "POST";
        case HttpMethod::Head:
            return "HEAD";
        case HttpMethod::Put:
            return "PUT";
        case HttpMethod::Delete:
            return "DELETE";
        case HttpMethod::Connect:
            return "CONNECT";
        case HttpMethod::Options:
            return "OPTIONS";
        case HttpMethod::Trace:
            return "TRACE";
        case HttpMethod::Patch:
            return "PATCH";
        default:
            __builtin_unreachable();
    }
}

std::optional<HttpMethod> parse_method(const std::string_view s) noexcept {
    if (s == "GET") {
        return HttpMethod::Get;
    }
    if (s == "POST") {
        return HttpMethod::Post;
    }
    if (s == "HEAD") {
        return HttpMethod::Head;
    }
    if (s == "PUT") {
        return HttpMethod::Put;
    }
    if (s == "DELETE") {
        return HttpMethod::Delete;
    }
    if (s == "CONNECT") {
        return HttpMethod::Connect;
    }
    if (s == "OPTIONS") {
        return HttpMethod::Options;
    }
    if (s == "TRACE") {
        return HttpMethod::Trace;
    }
    if (s == "PATCH") {
        return HttpMethod::Patch;
    }
    return std::nullopt;
}

Request::Request(HttpMethod method, std::string&& target, Headers&& headers,
                 std::string&& body)
    : method_{method},
      target_{std::move(target)},
      headers_{std::move(headers)},
      body_{std::move(body)} {}

Request::Request(Request&& rhs) noexcept
    : method_{rhs.method_},
      target_{std::move(rhs.target_)},
      headers_{std::move(rhs.headers_)},
      params_{std::move(rhs.params_)},
      body_{std::move(rhs.body_)} {}

Request& Request::operator=(Request&& rhs) noexcept {
    std::swap(method_, rhs.method_);
    std::swap(target_, rhs.target_);
    std::swap(headers_, rhs.headers_);
    std::swap(params_, rhs.params_);
    std::swap(body_, rhs.body_);
    return *this;
}

std::string_view Request::body() const noexcept {
    return body_;
}

std::string_view Request::target() const noexcept {
    return target_;
}

HttpMethod Request::method() const noexcept {
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
std::unique_ptr<Request> RequestBuilder::build() {
    return std::unique_ptr<Request>(new Request{
        method_, std::move(target_), std::move(headers_), std::move(body_)});
}

}  // namespace waxwing
