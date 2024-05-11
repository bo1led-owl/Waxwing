#pragma once

#include <cstdint>
#include <expected>
#include <string_view>

#include "io.hh"
#include "result.hh"
#include "router.hh"

namespace waxwing {
class Server final {
    internal::Router router_;
    internal::Socket socket_;

public:
    void route(
        HttpMethod method, std::string_view target,
        const std::function<std::unique_ptr<Response>()>& handler) noexcept;
    void route(HttpMethod method, std::string_view target,
               const std::function<std::unique_ptr<Response>(Request const&)>&
                   handler) noexcept;
    void route(
        HttpMethod method, std::string_view target,
        const std::function<std::unique_ptr<Response>(const PathParameters)>&
            handler) noexcept;
    void route(HttpMethod method, std::string_view target,
               const std::function<std::unique_ptr<Response>(
                   Request const&, const PathParameters)>& handler) noexcept;

    void set_not_found_handler(internal::RequestHandler handler);

    Result<void, std::string> bind(std::string_view address, uint16_t port,
                                   int backlog = 512) noexcept;

    void serve() const noexcept;
    void print_route_tree() const noexcept;
    void disable_logging() noexcept;
};
};  // namespace waxwing
