#pragma once

#include <cstdint>
#include <expected>
#include <string_view>

#include "io.hh"
#include "result.hh"
#include "router.hh"

namespace waxwing {
class Server {
    internal::Router router_;
    internal::Socket socket_;

public:
    bool route(
        HttpMethod method, std::string_view target,
        const std::function<std::unique_ptr<Response>()>& handler) noexcept;
    bool route(HttpMethod method, std::string_view target,
               const std::function<std::unique_ptr<Response>(Request const&)>&
                   handler) noexcept;
    bool route(HttpMethod method, std::string_view target,
               const std::function<std::unique_ptr<Response>(const Params)>&
                   handler) noexcept;
    bool route(HttpMethod method, std::string_view target,
               const std::function<std::unique_ptr<Response>(
                   Request const&, const Params)>& handler) noexcept;

    void set_not_found_handler(internal::RequestHandler handler);

    Result<void, std::string> bind(std::string_view address, uint16_t port,
                                   int backlog = 512) noexcept;

    void serve() const noexcept;
    void print_route_tree() const noexcept;
    void disable_logging() noexcept;
};
};  // namespace waxwing
