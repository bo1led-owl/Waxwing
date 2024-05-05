#pragma once

#include <cstdint>
#include <expected>
#include <string_view>

#include "result.hh"
#include "router.hh"

namespace http {
class Server {
    internal::Router router_;

public:
    void route(std::string_view target, Method method,
               const internal::RequestHandler& handler) noexcept;

    Result<void, std::string_view> serve(std::string_view address,
                                         uint16_t port) const noexcept;
    void print_route_tree() const noexcept;
};
};  // namespace http
