#include "waxwing/io.hh"

#include <arpa/inet.h>
#include <asm-generic/socket.h>
#include <fcntl.h>
#include <fmt/core.h>
#include <netinet/in.h>
#include <spdlog/spdlog.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cassert>
#include <cerrno>
#include <cstdint>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>

#include "waxwing/result.hh"

namespace waxwing::io {
Connection::~Connection() {
    // spdlog::debug("~Connection({})", fd_);
    if (fd_ > 0) {
        deleter_(fd_);
        close(fd_);
    }
}

Connection::Connection(Connection&& other) noexcept
    : fd_{std::exchange(other.fd_, -1)},
      addr_(std::exchange(other.addr_, sockaddr_in{})),
      len_(std::exchange(other.len_, socklen_t{})),
      deleter_(std::exchange(other.deleter_, {})) {}

Connection& Connection::operator=(Connection&& other) noexcept {
    std::swap(fd_, other.fd_);
    std::swap(addr_, other.addr_);
    std::swap(len_, other.len_);
    std::swap(deleter_, other.deleter_);
    return *this;
}

Acceptor::~Acceptor() { close(fd_); }

Result<Acceptor, std::string> Acceptor::create(const std::string_view address,
                                               const uint16_t port,
                                               const unsigned int backlog) {
    const int fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        return Error{std::make_error_code(std::errc{errno}).message()};
    }

    const int option = 1;
    ::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));

    int flags = fcntl(fd, F_GETFL, 0);
    assert(flags > 0);
    flags |= O_NONBLOCK;
    fcntl(fd, F_SETFL, flags);

    sockaddr_in addr{.sin_family = AF_INET, .sin_port = ::htons(port)};

    const int address_conversion_result =
        inet_pton(AF_INET, address.data(), &addr.sin_addr);
    if (address_conversion_result == 0) {
        return Error{fmt::format("Could not convert `{}` into internet address",
                                 address)};
    } else if (address_conversion_result < 0) {
        return Error{std::make_error_code(std::errc{errno}).message()};
    }

    if (::bind(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        return Error{std::make_error_code(std::errc{errno}).message()};
    }
    if (::listen(fd, backlog) < 0) {
        return Error{std::make_error_code(std::errc{errno}).message()};
    }

    return Acceptor{fd};
}

Acceptor::Acceptor(Acceptor&& other) noexcept
    : fd_{std::exchange(other.fd_, -1)} {}

Acceptor& Acceptor::operator=(Acceptor&& other) noexcept {
    std::swap(fd_, other.fd_);
    return *this;
}
}  // namespace waxwing::io
