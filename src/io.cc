#include "waxwing/io.hh"

#include <arpa/inet.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstdint>
#include <cstring>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>

#include "waxwing/result.hh"

namespace waxwing {
namespace internal {
FileDescriptor::~FileDescriptor() {
    close(fd_);
}

bool FileDescriptor::is_valid() const {
    return fd_ >= 0;
}

Connection::Connection(Connection&& other) noexcept
    : FileDescriptor{other.fd_} {
    other.fd_ = -1;
}

Connection& Connection::operator=(Connection&& rhs) noexcept {
    std::swap(fd_, rhs.fd_);
    return *this;
}

size_t Connection::recv(std::string& s, const size_t n) const {
    const size_t prev_size = s.size();

    s.reserve(s.size() + n);

    const size_t bytes_read = ::recv(fd_, &s[prev_size], n * sizeof(char), 0);

    s.resize(prev_size + bytes_read);
    return bytes_read;
}

size_t Connection::send(const std::span<char> s) const {
    return ::send(fd_, s.data(), s.size(), 0);
}

Result<Socket, std::string> Socket::create(const std::string_view address,
                                           const uint16_t port,
                                           const int backlog) {
    const int fd = ::socket(PF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        return Error{std::make_error_code(std::errc{errno}).message()};
    }

    const int option = 1;
    ::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));

    sockaddr_in addr{.sin_family = AF_INET,
                     .sin_port = ::htons(port),
                     .sin_addr = {.s_addr = ::htonl(INADDR_ANY)},
                     .sin_zero = {}};
    if (inet_pton(AF_INET, address.data(), &addr.sin_addr.s_addr) < 0) {
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

Socket::Socket(Socket&& other) noexcept : FileDescriptor{other.fd_} {
    other.fd_ = -1;
}

Socket& Socket::operator=(Socket&& rhs) noexcept {
    std::swap(fd_, rhs.fd_);
    return *this;
}

Connection Socket::accept() const {
    struct sockaddr_in clientaddr;
    socklen_t clientaddr_len = sizeof(clientaddr);
    return ::accept(fd_, reinterpret_cast<struct sockaddr*>(&clientaddr),
                    &clientaddr_len);
}
}  // namespace internal
}  // namespace waxwing
