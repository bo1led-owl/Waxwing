#include "waxwing/request.hh"

namespace waxwing {
Request::Request(HttpMethod method, std::string&& target, Headers&& headers,
                 std::string&& body)
    : method_{method},
      target_{std::move(target)},
      headers_{std::move(headers)},
      body_{std::move(body)} {}

std::string_view Request::body() const noexcept { return body_; }
std::string_view Request::target() const noexcept { return target_; }
HttpMethod Request::method() const noexcept { return method_; }

std::optional<std::string_view> Request::header(
    const std::string_view key) const noexcept {
    return headers_.get(key);
}

Request RequestBuilder::build() && {
    return {method_, std::move(target_), std::move(headers_), std::move(body_)};
}
}  // namespace waxwing
