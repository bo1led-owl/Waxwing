#pragma once

#include "file_descriptor.hh"
#include "router.hh"

namespace http {
class Server {
    Router router_;

    void handle_connection(const Connection& connection) const;

public:
    void route(const std::string_view target, const Method method,
               const RequestHandler& handler);

    bool serve(const std::string_view address, const uint16_t port) const;
    void print_route_tree() const;
};
};  // namespace http
