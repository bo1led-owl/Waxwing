#include "waxwing/request.hh"

#include "str_util.hh"

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
    const auto result = headers_.find(str_util::to_lower(key));
    if (result == headers_.end()) {
        return std::nullopt;
    }

    return result->second;
}

std::unique_ptr<Request> RequestBuilder::build() && {
    return std::unique_ptr<Request>(new Request{
        method_, std::move(target_), std::move(headers_), std::move(body_)});
}
}  // namespace waxwing
