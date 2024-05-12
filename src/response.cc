#include "waxwing/response.hh"

#include <sys/socket.h>

namespace waxwing {
Response::Response(Response&& rhs) noexcept
    : status_code_{rhs.status_code_},
      headers_{std::move(rhs.headers_)},
      body_{std::move(rhs.body_)} {}

Response& Response::operator=(Response&& rhs) noexcept {
    std::swap(status_code_, rhs.status_code_);
    std::swap(headers_, rhs.headers_);
    std::swap(body_, rhs.body_);
    return *this;
}

HttpStatusCode Response::status() const noexcept {
    return status_code_;
}

Headers& Response::headers() & noexcept {
    return headers_;
}

std::optional<std::string>& Response::body() & noexcept {
    return body_;
}

std::unique_ptr<Response> ResponseBuilder::build() {
    return std::unique_ptr<Response>(
        new Response{status_code_, std::move(headers_), std::move(body_)});
}

}  // namespace waxwing
