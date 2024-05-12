#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>

#include "waxwing/result.hh"

namespace waxwing::internal {
class Connection final {
    int fd_;

public:
    explicit Connection(const int fd) : fd_{fd} {}
    ~Connection();

    Connection(const Connection&) = delete;
    Connection& operator=(const Connection&) = delete;

    Connection(Connection&&) noexcept;
    Connection& operator=(Connection&&) noexcept;

    size_t recv(std::string& s, size_t n) const;
    size_t send(std::span<char> s) const;

    bool is_valid() const noexcept;
};

class Socket final {
    int fd_;

    explicit Socket(const int fd) : fd_{fd} {}

public:
    Socket() : fd_{-1} {}
    ~Socket();

    static Result<Socket, std::string> create(std::string_view address,
                                              uint16_t port, int backlog);

    Socket(const Socket&) = delete;
    Socket& operator=(const Socket&) = delete;

    Socket(Socket&&) noexcept;
    Socket& operator=(Socket&&) noexcept;

    Connection accept() const;
};
}  // namespace waxwing::internal
