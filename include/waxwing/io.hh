#pragma once

#include <fcntl.h>
#include <fmt/core.h>
#include <netinet/in.h>
#include <spdlog/spdlog.h>
#include <stdio.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <cassert>
#include <cerrno>
#include <coroutine>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <unordered_map>
#include <utility>
#include <vector>

#include "waxwing/dispatcher.hh"
#include "waxwing/movable_function.hh"
#include "waxwing/result.hh"

namespace waxwing::io {
using async::Dispatcher;
using internal::MovableFunction;

class EpollIOService;

class Connection final {
    int fd_;
    sockaddr_in addr_;
    socklen_t len_;
    MovableFunction<void(int)> deleter_;

    Connection(int fd, sockaddr_in addr, socklen_t len,
               MovableFunction<void(int)> deleter)
        : fd_{fd}, addr_{addr}, len_{len}, deleter_{std::move(deleter)} {
        assert(deleter_);
    }

public:
    friend class EpollIOService;
    Connection() : fd_{-1} {}

    ~Connection();

    int fd() const noexcept { return fd_; }
    const sockaddr_in& addr() const noexcept { return addr_; }

    Connection(const Connection&) = delete;
    Connection& operator=(const Connection&) = delete;

    Connection(Connection&&) noexcept;
    Connection& operator=(Connection&&) noexcept;
};

class Acceptor final {
    int fd_;

    explicit Acceptor(int fd) : fd_{fd} {}

public:
    friend class EpollIOService;

    static Result<Acceptor, std::string> create(std::string_view address,
                                                uint16_t port,
                                                unsigned int backlog = 2048);

    Acceptor() : fd_{-1} {}
    ~Acceptor();

    Acceptor(const Acceptor&) = delete;
    Acceptor& operator=(const Acceptor&) = delete;

    Acceptor(Acceptor&&) noexcept;
    Acceptor& operator=(Acceptor&&) noexcept;
};

class EpollIOService final {
    static constexpr size_t MAX_EVENTS = 100;

    std::vector<epoll_event> events_;
    std::unordered_map<int, std::vector<std::coroutine_handle<>>> subscribers_;
    int epoll_fd_{-1};
    std::mutex mut_{};

public:
    EpollIOService() : events_(MAX_EVENTS) {
        const int fd = epoll_create1(0);
        if (fd < 0) {
            throw std::runtime_error{
                fmt::format("Error initializing epoll: {}",
                            std::make_error_code(std::errc{errno}).message())};
        }

        epoll_fd_ = fd;
    }

    ~EpollIOService() { close(epoll_fd_); }

    void subscribe(int fd, std::coroutine_handle<> h,
                   unsigned int events = EPOLLIN | EPOLLOUT) {
        epoll_event event{.events = events, .data = {.fd = fd}};

        std::lock_guard<std::mutex> l{mut_};
        auto it = subscribers_.find(fd);
        if (it == subscribers_.end()) {
            int result = epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, &event);

            if (result < 0) {
                throw std::runtime_error{fmt::format(
                    "Error in `epoll_ctl`: {}",
                    std::make_error_code(std::errc{errno}).message())};
            }

            spdlog::debug("added {}", fd);
            subscribers_.emplace(fd, std::vector<std::coroutine_handle<>>{h});
        } else {
            it->second.push_back(h);
        }
        spdlog::debug("{}: subscribed to {}", h.address(), fd);
    }

    void remove_fd(int fd) {
        if (fd < 0) {
            return;
        }

        std::lock_guard<std::mutex> l{mut_};
        epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, nullptr);

        spdlog::debug("removing {}", fd);

        assert(subscribers_[fd].empty());
        subscribers_.erase(fd);
    }

    auto read(int fd, std::span<char> buf) {
        assert(buf.data());

        class ReadAwaiter {
            EpollIOService& service_;
            int fd_;
            std::span<char> buf_;

        public:
            ReadAwaiter(EpollIOService& service, int fd, std::span<char> buf)
                : service_{service}, fd_{fd}, buf_{buf} {}

            bool await_ready() const noexcept { return false; }
            void await_suspend(std::coroutine_handle<> h) {
                service_.subscribe(fd_, h);
            }
            Result<ssize_t, std::errc> await_resume() {
                spdlog::debug("read");
                ssize_t result = ::read(fd_, buf_.data(), buf_.size());
                if (result < 0) {
                    return Error{std::errc{errno}};
                }

                return result;
            }
        };

        return ReadAwaiter{*this, fd, buf};
    }

    auto recv(Connection const& conn, std::span<char> buf) {
        assert(buf.data());

        class RecvAwaiter {
            EpollIOService& service_;
            int fd_;
            std::span<char> buf_;

        public:
            RecvAwaiter(EpollIOService& service, int fd, std::span<char> buf)
                : service_{service}, fd_{fd}, buf_{buf} {}

            bool await_ready() const noexcept { return false; }
            void await_suspend(std::coroutine_handle<> h) {
                service_.subscribe(fd_, h);
            }
            Result<ssize_t, std::string> await_resume() {
                // spdlog::debug("recv'd");

                ssize_t result;
                do {
                    result = ::recv(fd_, buf_.data(), buf_.size(), 0);
                } while (result == -EAGAIN || result == -EWOULDBLOCK);
                if (result < 0) {
                    return Error{
                        std::make_error_code(std::errc{errno}).message()};
                }

                return result;
            }
        };

        return RecvAwaiter{*this, conn.fd_, buf};
    }

    auto accept(Acceptor const& acc) {
        class AcceptAwaiter {
            EpollIOService& service_;
            int fd_;

        public:
            AcceptAwaiter(EpollIOService& service, int fd)
                : service_{service}, fd_{fd} {}

            bool await_ready() const noexcept { return false; }
            void await_suspend(std::coroutine_handle<> h) {
                service_.subscribe(fd_, h, EPOLLIN);
            }
            Result<Connection, std::errc> await_resume() {
                sockaddr_in clientaddr{};
                socklen_t clientaddr_len = sizeof(clientaddr);
                int fd = ::accept(fd_, reinterpret_cast<sockaddr*>(&clientaddr),
                                  &clientaddr_len);
                if (fd < 0) {
                    return Error{std::errc{errno}};
                }

                int flags = fcntl(fd, F_GETFL, 0);
                assert(flags >= 0);
                flags |= O_NONBLOCK;
                fcntl(fd, F_SETFL, flags);

                // spdlog::debug("accepted");
                EpollIOService* s = &service_;
                return Connection{fd, clientaddr, clientaddr_len,
                                  [s](int fd) { s->remove_fd(fd); }};
            }
        };

        return AcceptAwaiter{*this, acc.fd_};
    }

    auto send(Connection const& conn, std::span<char const> data) {
        assert(data.data());

        class SendAwaiter {
            EpollIOService& service_;
            int fd_;
            std::span<char const> data_;

        public:
            SendAwaiter(EpollIOService& service, int fd,
                        std::span<char const> data)
                : service_{service}, fd_{fd}, data_{data} {}

            bool await_ready() const noexcept { return false; }
            void await_suspend(std::coroutine_handle<> h) {
                service_.subscribe(fd_, h);
            }
            Result<ssize_t, std::errc> await_resume() {
                // spdlog::debug("sent");
                ssize_t result;
                do {
                    result = ::send(fd_, data_.data(), data_.size(), MSG_DONTWAIT);
                } while (result == -EAGAIN || result == -EWOULDBLOCK);
                if (result < 0) {
                    return Error{std::errc{errno}};
                }

                return result;
            }
        };

        return SendAwaiter{*this, conn.fd_, data};
    }

    void run(Dispatcher& dispatcher) {
        for (;;) {
            // spdlog::debug("waiting, {} subs", subscribers_.size());
            int nfds = epoll_wait(epoll_fd_, events_.data(), MAX_EVENTS, -1);
            if (nfds == -1) {
                throw std::runtime_error{fmt::format(
                    "Error in `epoll_wait`: {}",
                    std::make_error_code(std::errc{errno}).message())};
            }

            // spdlog::debug("woke up");

            if (nfds > 0) {
                std::lock_guard<std::mutex> l{mut_};
                for (int i = 0; i < nfds; ++i) {
                    const int fd = events_[i].data.fd;

                    const auto it = subscribers_.find(fd);
                    // assert(range.first != awaiting_coroutines_.end());
                    // spdlog::debug("fd: {} | dist: {}", fd,
                    //              std::distance(range.first, range.second));
                    if (it != subscribers_.end()) {
                        for (const std::coroutine_handle<> h : it->second) {
                            dispatcher.async(h);
                        }
                        it->second.clear();
                    }
                }
            }
        }
    }
};
}  // namespace waxwing::io
