#include "waxwing/io.hh"

#include <arpa/inet.h>
#include <spdlog/spdlog.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstdint>
#include <cstring>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>

#include "waxwing/result.hh"

namespace waxwing::internal {
Connection::~Connection() {
    close(fd_);
}

Connection::Connection(Connection&& other) noexcept
    : fd_{std::exchange(other.fd_, -1)} {}

Connection& Connection::operator=(Connection&& rhs) noexcept {
    std::swap(fd_, rhs.fd_);
    return *this;
}

size_t Connection::recv(std::string& s, const size_t n) const {
    std::vector<char> buffer(n);

    const size_t bytes_read = ::recv(fd_, buffer.data(), n * sizeof(char), 0);

    buffer.resize(bytes_read);
    s.append(buffer.cbegin(), buffer.cend());
    return bytes_read;
}

size_t Connection::send(const std::span<char> s) const {
    return ::send(fd_, s.data(), s.size(), 0);
}

bool Connection::is_valid() const noexcept {
    return fd_ >= 0;
}

Socket::~Socket() {
    close(fd_);
}

Result<Socket, std::string> Socket::create(const std::string_view address,
                                           const uint16_t port,
                                           const int backlog) {
    const int fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        return Error{std::make_error_code(std::errc{errno}).message()};
    }

    const int option = 1;
    ::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));

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

    return Socket{fd};
}

Socket::Socket(Socket&& other) noexcept : fd_{std::exchange(other.fd_, -1)} {}

Socket& Socket::operator=(Socket&& rhs) noexcept {
    std::swap(fd_, rhs.fd_);
    return *this;
}

Connection Socket::accept() const {
    sockaddr_in clientaddr{};
    socklen_t clientaddr_len = sizeof(clientaddr);
    int result = ::accept(fd_, reinterpret_cast<sockaddr*>(&clientaddr),
                          &clientaddr_len);

    return Connection{result};
}
}  // namespace waxwing::internal
