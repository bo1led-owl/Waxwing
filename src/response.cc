#include "waxwing/response.hh"

#include <sys/socket.h>

namespace waxwing {
HttpStatusCode Response::status() const noexcept { return status_code_; }

Headers& Response::headers() & noexcept { return headers_; }

std::optional<std::string>& Response::body() & noexcept { return body_; }

std::unique_ptr<Response> ResponseBuilder::build() && {
    return std::unique_ptr<Response>(
        new Response{status_code_, std::move(headers_), std::move(body_)});
}

}  // namespace waxwing
