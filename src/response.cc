#include "waxwing/response.hh"

#include <optional>
#include <string_view>
#include <utility>

#include "waxwing/http.hh"

namespace waxwing {
HttpStatusCode Response::status() const noexcept { return status_code_; }
Headers& Response::headers() & noexcept { return headers_; }
std::optional<std::string_view> Response::body() const noexcept {
    return body_;
}

Response ResponseBuilder::build() && {
    return {status_code_, std::move(headers_), std::move(body_)};
}

}  // namespace waxwing
