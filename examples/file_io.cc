#include <cstdio>
#include <iostream>

#include "waxwing/async.hh"
#include "waxwing/dispatcher.hh"
#include "waxwing/io.hh"

std::atomic<bool> finished = false;

waxwing::async::Task<> get_from_file(waxwing::async::Dispatcher& disp,
                                     waxwing::io::EpollIOService& io, int fd) {
    spdlog::debug("1");
    std::vector<char> data(1024);
    spdlog::debug("2");
    data.resize((co_await io.read(fd, std::span{data})).value());
    spdlog::debug("3");
    // data.resize(read(fd, data.data(), data.size()));
    std::cout << std::string{data.begin(), data.end()};
    finished = true;

    co_return;
}

int main() {
    spdlog::set_level(spdlog::level::debug);

    const int epoll_fd = epoll_create1(0);

    FILE* f = fopen("foo.txt", "r");
    if (!f) {
        perror("fopen");
        return -1;
    }

    int fd = fileno(f);

    int flags = fcntl(fd, F_GETFL, 0);
    flags |= O_NONBLOCK;
    fcntl(fd, F_SETFL, flags);

    auto ev = epoll_event{.events = EPOLLIN, .data{.fd = fd}};
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev) < 0) {
        perror("epoll_ctl");
        return -1;
    }
    
    std::vector<epoll_event> events(10);
    for (;;) {
        int n = epoll_wait(epoll_fd, events.data(), events.size(), -1);

        if (n > 0) {
            char buf[100];
            read(fd, buf, sizeof(buf));
            std::cout << buf << '\n';
            break;
        }
    }

    close(epoll_fd);
}
