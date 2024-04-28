#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

#include "result.hh"

namespace http {
namespace internal {
class FileDescriptor {
protected:
    int fd_;

    FileDescriptor() : fd_(-1) {}

public:
    FileDescriptor(const int fd) : fd_{fd} {}
    virtual ~FileDescriptor();

    FileDescriptor(const FileDescriptor&) = delete;
    FileDescriptor& operator=(const FileDescriptor&) = delete;

    bool is_valid() const;
};

class Connection final : public FileDescriptor {
public:
    Connection(const int fd) : FileDescriptor{fd} {}

    Connection(const Connection&) = delete;
    Connection& operator=(const Connection&) = delete;

    Connection(Connection&&) noexcept;
    Connection& operator=(Connection&&) noexcept;

    void recv(std::string& s, size_t n) const;
    size_t send(std::string_view s) const;
};

class Socket final : public FileDescriptor {
    Socket(int fd) : FileDescriptor{fd} {}

public:
    Socket() : FileDescriptor{-1} {}

    static Result<Socket, std::string_view> create(std::string_view address,
                                                   uint16_t port);

    Socket(const Socket&) = delete;
    Socket& operator=(const Socket&) = delete;

    Socket(Socket&&) noexcept;
    Socket& operator=(Socket&&) noexcept;

    Connection accept() const;
};
}  // namespace internal
}  // namespace http
