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
               const internal::RequestHandler& handler);

    Result<void, std::string_view> serve(std::string_view address,
                                         uint16_t port) const;
    void print_route_tree() const;
};
};  // namespace http
