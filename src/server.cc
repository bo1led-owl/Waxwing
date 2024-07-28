#include "waxwing/server.hh"

#include <fmt/core.h>
#include <spdlog/spdlog.h>
#include <sys/types.h>

#include <charconv>
#include <coroutine>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <list>
#include <mutex>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <system_error>
#include <thread>
#include <utility>
#include <vector>

#include "waxwing/async.hh"
#include "waxwing/dispatcher.hh"
#include "waxwing/http.hh"
#include "waxwing/io.hh"
#include "waxwing/request.hh"
#include "waxwing/response.hh"
#include "waxwing/result.hh"
#include "waxwing/router.hh"
#include "waxwing/str_split.hh"
#include "waxwing/str_util.hh"

namespace waxwing {
using async::Dispatcher;
using async::Task;
using internal::MovableFunction;
using internal::RequestHandler;
using internal::Router;
using internal::RouteTarget;
using internal::RoutingResult;
using io::Acceptor;
using io::Connection;
using io::EpollIOService;

namespace {
std::pair<std::string_view, std::string_view> parse_header(
    const std::string_view line) {
    const size_t colon_pos = line.find(':');

    return {str_util::trim(line.substr(0, colon_pos)),
            str_util::trim(
                line.substr(colon_pos + 1, line.size() - colon_pos - 1))};
}

void concat_header(std::string& buf, const std::string_view key,
                   const std::string_view value) {
    buf += str_util::trim(key);
    buf += ": ";
    buf += str_util::trim(value);
    buf += "\r\n";
}

Task<Result<std::string, std::string>> read_body(Dispatcher& disp,
                                                 EpollIOService& io,
                                                 const Connection& conn,
                                                 std::string_view initial,
                                                 const size_t length) {
    constexpr size_t CHUNK_SIZE = 1024;

    std::string body{initial};
    size_t bytes_read = initial.size();
    do {
        body.resize(body.size() + CHUNK_SIZE);
        auto cur_read = co_await io.recv(conn, std::span{body});
        if (cur_read.has_value()) {
            co_return Error{std::move(cur_read.error())};
        }
        body.resize(body.size() + cur_read.value());
        bytes_read += cur_read.value();
    } while (bytes_read < length);

    co_return body;
}

std::optional<std::pair<HttpMethod, std::string>> parse_request_line(
    std::string_view line) {
    auto token_split = str_util::split(line, ' ');

    std::optional<HttpMethod> method =
        and_then(token_split.next(),
                 [](const std::string_view s) { return parse_method(s); });
    std::optional<std::string> target = map(
        token_split.next(), [](std::string_view s) { return std::string{s}; });
    const std::optional<std::string_view> http_version = token_split.next();

    if (!method || !target || !http_version) {
        return std::nullopt;
    }
    return {{method.value(), std::move(*target)}};
}

Result<size_t, std::string> parse_content_length(std::string_view raw) {
    size_t content_length = 0;

    const std::errc error_code =
        std::from_chars(raw.cbegin(), raw.cend(), content_length).ec;
    if (error_code != std::errc{}) {
        return Error{fmt::format("couldn't parse `Content-Length`: {}",
                                 std::make_error_code(error_code).message())};
    } else {
        return content_length;
    }
}

Task<Result<Request, std::string>> read_request(Dispatcher& disp,
                                                EpollIOService& io,
                                                const Connection& conn) {
    constexpr size_t HEADERS_BUFFER_SIZE = 2048;  // 2 Kb

    // std::string buf;
    // buf.resize(HEADERS_BUFFER_SIZE);
    std::vector<char> buf(HEADERS_BUFFER_SIZE);
    auto size = co_await io.recv(conn, std::span{buf});
    if (size.has_error()) {
        co_return Error{std::move(size.error())};
    }
    buf.resize(size.value());

    str_util::Split line_split =
        str_util::split(std::string_view{buf.begin(), buf.end()}, "\r\n");

    std::optional<std::pair<HttpMethod, std::string>> request_line_opt =
        and_then(line_split.next(), [](const std::string_view line) {
            return parse_request_line(line);
        });
    if (!request_line_opt) {
        co_return Error{"Error parsing request line"};
    }

    auto [method, target] = std::move(*request_line_opt);

    RequestBuilder builder{method, std::move(target)};
    Headers headers;

    for (std::optional<std::string_view> line = line_split.next();
         line.has_value(); line = line_split.next()) {
        if (line->empty()) {
            break;
        }

        std::pair<std::string_view, std::string_view> header =
            parse_header(*line);
        headers.insert_or_assign(std::string{header.first},
                                 std::string{header.second});
    };

    // requests with DELETE, POST, PATCH or PUT method generally have a body
    // https://developer.mozilla.org/en-US/docs/Web/HTTP/Methods
    const bool has_body =
        method == HttpMethod::Delete || method == HttpMethod::Patch ||
        method == HttpMethod::Post || method == HttpMethod::Put ||
        headers.contains("content-length") || headers.contains("content-type");
    if (has_body) {
        auto raw_content_length_opt = headers.get("content-length");
        if (raw_content_length_opt.has_value()) {
            Result<size_t, std::string> content_length =
                parse_content_length(*raw_content_length_opt);
            if (!content_length) {
                co_return Error{content_length.error()};
            }

            auto body_res = co_await read_body(
                disp, io, conn, line_split.remaining(), *content_length);
            if (body_res.has_error()) {
                co_return Error{std::move(body_res.error())};
            }

            builder.body(std::move(body_res.value()));
        } else {
            builder.body(line_split.remaining());
        }
    }

    builder.headers(std::move(headers));
    co_return std::move(builder).build();
}

Task<Result<void, std::string>> send_response(Dispatcher& disp,
                                              EpollIOService& io,
                                              const Connection& conn,
                                              Response resp) noexcept {
    constexpr std::string_view http_version = "HTTP/1.1";
    std::string buf = fmt::format("{} {}\r\n", http_version,
                                  format_status_code(resp.status()));

    Headers& headers = resp.headers();

    const std::optional<std::string_view> body_opt = resp.body();
    if (body_opt.has_value()) {
        std::string content_length = std::to_string(body_opt->size());
        headers.insert_or_assign("Content-Length", std::move(content_length));
    }

    headers.insert_or_assign("Connection", "Close");

    for (auto [key, value] : headers) {
        concat_header(buf, key, value);
    }

    // empty line is required even if the body is empty
    buf += "\r\n";

    if (body_opt.has_value()) {
        buf += *body_opt;
    }

    size_t sent = 0;
    while (sent < buf.size()) {
        auto res = co_await io.send(conn, buf.substr(sent));
        if (res.has_error()) {
            co_return Error{std::make_error_code(res.error()).message()};
        }
        sent += res.value();
    }

    co_return {};
}

class ConnectionHandler final {
    using It = std::list<ConnectionHandler>::iterator;
    std::optional<Task<>> t_;
    It iter_;
    MovableFunction<void(It)> deleter_;
    Connection connection_;

    Task<> get_coro(Dispatcher& disp, EpollIOService& io,
                    const Router& router) noexcept {
        auto read_result = co_await read_request(disp, io, connection_);
        if (!read_result) {
            spdlog::error("Error: `{}`", read_result.error());
            co_return;
        }

        const Request& req = read_result.value();
        const RoutingResult route = router.route(req.method(), req.target());

        Response resp = route.handler()(req, route.parameters());

        auto send_result =
            co_await send_response(disp, io, connection_, std::move(resp));
        if (send_result.has_error()) {
            spdlog::error("Error: `{}`", send_result.error());
            co_return;
        }
        spdlog::info("{} {} -> {}", format_method(req.method()), req.target(),
                     format_status_code(resp.status()));
    }

public:
    ConnectionHandler(MovableFunction<void(It)> deleter)
        : deleter_{std::move(deleter)} {}
    void set_iter(It it) noexcept { iter_ = it; }

    std::coroutine_handle<> start(Dispatcher& disp, EpollIOService& io,
                                  const Router& router, Connection connection) {
        connection_ = std::move(connection);
        t_ = get_coro(disp, io, router);
        t_->set_deleter([this]() { deleter_(iter_); });
        return t_->handle();
    }
};

Task<> loop(Dispatcher& disp, EpollIOService& io, const Router& router,
            const Acceptor& acc) {
    std::list<ConnectionHandler> connections;
    std::mutex m;
    for (;;) {
        auto connection = co_await io.accept(acc);
        if (connection) {
            spdlog::info("accepted {}", connection->addr().sin_addr.s_addr);
            ConnectionHandler handler{
                {[&connections, &m](std::list<ConnectionHandler>::iterator i) {
                    std::lock_guard<std::mutex> l{m};
                    connections.erase(i);
                    spdlog::debug("-1: {} connections", connections.size());
                }}};

            // begin critical section
            m.lock();

            ConnectionHandler& h_ref =
                connections.emplace_front(std::move(handler));
            h_ref.set_iter(connections.begin());

            spdlog::debug("+1: {} connections", connections.size());

            m.unlock();
            // end critical section

            disp.async(
                h_ref.start(disp, io, router, std::move(connection.value())));
        } else {
            spdlog::error("`accept` error: {}",
                          std::make_error_code(connection.error()).message());
        }
    }
}
}  // namespace

void Server::route(const HttpMethod method, const RouteTarget target,
                   const std::function<Response()>& handler) noexcept {
    route(method, target, [handler](const Request&, const PathParameters) {
        return handler();
    });
}

void Server::route(
    const HttpMethod method, const RouteTarget target,
    const std::function<Response(Request const&)>& handler) noexcept {
    route(method, target, [handler](const Request& req, const PathParameters) {
        return handler(req);
    });
}

void Server::route(
    const HttpMethod method, const RouteTarget target,
    const std::function<Response(const PathParameters)>& handler) noexcept {
    route(method, target,
          [handler](const Request&, const PathParameters params) {
              return handler(params);
          });
}

void Server::route(const HttpMethod method, const RouteTarget target,
                   const RequestHandler& handler) noexcept {
    router_.add_route(method, target, handler);
}

void Server::print_route_tree() const noexcept { router_.print_tree(); }

void Server::set_not_found_handler(RequestHandler handler) {
    router_.set_not_found_handler(handler);
}

Result<void, std::string> Server::bind(const std::string_view address,
                                       const uint16_t port,
                                       const int backlog) noexcept {
    auto acc_res = Acceptor::create(address, port, backlog);
    if (!acc_res) {
        return Error{std::move(acc_res.error())};
    }
    this->acceptor_ = std::move(acc_res.value());

    return {};
}

void Server::serve() const noexcept {
    const unsigned WORKER_THREADS = std::thread::hardware_concurrency() - 1;

    Dispatcher disp{WORKER_THREADS};
    EpollIOService io;

    Task<> t = loop(disp, io, router_, acceptor_);
    disp.async(t.handle());

    io.run(disp);
}
}  // namespace waxwing
