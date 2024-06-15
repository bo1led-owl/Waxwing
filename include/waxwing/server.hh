#pragma once

#include <cstdint>
#include <string_view>

#include "waxwing/io.hh"
#include "waxwing/result.hh"
#include "waxwing/router.hh"

namespace waxwing {
class Server final {
    internal::Router router_;
    internal::Socket socket_;

public:
    void route(HttpMethod method, internal::RouteTarget target,
               const internal::RequestHandler& handler) noexcept;
    void route(HttpMethod method, internal::RouteTarget target,
               const std::function<Response()>& handler) noexcept;
    void route(HttpMethod method, internal::RouteTarget target,
               const std::function<Response(Request const&)>& handler) noexcept;
    void route(
        HttpMethod method, internal::RouteTarget target,
        const std::function<Response(const PathParameters)>& handler) noexcept;

    void set_not_found_handler(internal::RequestHandler handler);

    Result<void, std::string> bind(std::string_view address, uint16_t port,
                                   int backlog = 100) noexcept;

    void serve() const noexcept;
    void print_route_tree() const noexcept;
};
};  // namespace waxwing
