#pragma once

#include <spdlog/spdlog.h>

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
    bool route(Method method, std::string_view target,
               const internal::RequestHandler& handler) noexcept;

    Result<void, std::string> bind(std::string_view address,
                                   uint16_t port) noexcept;

    void serve() const noexcept;
    void print_route_tree() const noexcept;
    void disable_logging() noexcept;
};
};  // namespace waxwing
