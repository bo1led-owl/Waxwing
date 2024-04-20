#pragma once

#include "file_descriptor.hh"
#include "result.hh"
#include "router.hh"

namespace http {
class Server {
    Router router_;

    Result<void, std::string_view> handle_connection(
        const Connection& connection) const;

public:
    void route(const std::string_view target, const Method method,
               const RequestHandler& handler);

    Result<void, std::string_view> serve(const std::string_view address,
                                         const uint16_t port) const;
    void print_route_tree() const;
};
};  // namespace http
