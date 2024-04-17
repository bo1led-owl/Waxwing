#include "file_descriptor.hh"

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

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

Socket::Socket(const std::string_view address, const uint16_t port) {
    const auto report_on_failure = [this](const int result,
                                          const std::string_view msg) {
        if (result < 0) {
            this->fd_ = -1;
            ::perror(msg.data());
            return true;
        }
        return false;
    };

    fd_ = ::socket(PF_INET, SOCK_STREAM, 0);
    if (report_on_failure(fd_, "Socket creation error")) {
        return;
    }

    const int option = 1;
    ::setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));

    sockaddr_in addr{.sin_family = AF_INET,
                     .sin_port = ::htons(port),
                     .sin_addr = {.s_addr = ::htonl(INADDR_ANY)},
                     .sin_zero = {}};
    if (report_on_failure(
            inet_pton(AF_INET, address.data(), &addr.sin_addr.s_addr),
            "Converting address failure")) {
        return;
    }

    if (report_on_failure(
            ::bind(fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)),
            "Socket binding error")) {
        return;
    }
    if (report_on_failure(::listen(fd_, MAX_CONNECTIONS),
                          "Socket listening error")) {
        return;
    }
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
