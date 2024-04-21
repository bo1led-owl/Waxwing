#include "file_descriptor.hh"

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>
#include <string>
#include <utility>

namespace http {
constexpr int MAX_CONNECTIONS = 512;

FileDescriptor::~FileDescriptor() {
    close(fd_);
}

bool FileDescriptor::is_valid() const {
    return fd_ >= 0;
}

Connection::Connection(Connection&& other) : FileDescriptor{other.fd_} {
    other.fd_ = -1;
}

Connection& Connection::operator=(Connection&& rhs) {
    std::swap(fd_, rhs.fd_);
    return *this;
}

void Connection::recv(std::string& s, const size_t n) const {
    const size_t prev_size = s.size();
    s.resize(s.size() + n);
    s.resize(prev_size + ::recv(fd_, &s[prev_size], n * sizeof(char), 0));
}

size_t Connection::send(const std::string_view s) const {
    return ::send(fd_, s.data(), s.size(), 0);
}

Result<Socket, std::string_view> Socket::create(const std::string_view address,
                                                const uint16_t port) {
    const int fd = ::socket(PF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        return Error{strerror(errno)};
    }

    const int option = 1;
    ::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));

    sockaddr_in addr{.sin_family = AF_INET,
                     .sin_port = ::htons(port),
                     .sin_addr = {.s_addr = ::htonl(INADDR_ANY)},
                     .sin_zero = {}};
    if (inet_pton(AF_INET, address.data(), &addr.sin_addr.s_addr) < 0) {
        return Error{strerror(errno)};
    }

    if (::bind(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        return Error{strerror(errno)};
    }
    if (::listen(fd, MAX_CONNECTIONS)) {
        return Error{strerror(errno)};
    }

    return Socket{fd};
}

Socket::Socket(Socket&& other) : FileDescriptor{other.fd_} {
    other.fd_ = -1;
}

Socket& Socket::operator=(Socket&& rhs) {
    std::swap(fd_, rhs.fd_);
    return *this;
}

Connection Socket::accept() const {
    struct sockaddr_in clientaddr;
    socklen_t clientaddr_len = sizeof(clientaddr);
    return ::accept(fd_, reinterpret_cast<struct sockaddr*>(&clientaddr),
                    &clientaddr_len);
}
}  // namespace http
