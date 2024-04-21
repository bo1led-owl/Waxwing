#pragma once

#include <cstdint>
#include <string_view>

#include "file_descriptor.hh"
#include "result.hh"
#include "router.hh"

namespace http {
class Server {
    Router router_;

    Result<void, std::string_view> handle_connection(
        Connection connection) const;

public:
    void route(std::string_view target, Method method,
               const RequestHandler& handler);

    Result<void, std::string_view> serve(std::string_view address,
                                         uint16_t port) const;
    void print_route_tree() const;
};
};  // namespace http
