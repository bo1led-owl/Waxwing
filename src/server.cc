#include "waxwing/server.hh"

#include <fmt/core.h>
#include <spdlog/spdlog.h>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>

#include "thread_pool.hh"
#include "waxwing/http.hh"
#include "waxwing/io.hh"
#include "waxwing/request.hh"
#include "waxwing/response.hh"
#include "waxwing/router.hh"
#include "waxwing/str_split.hh"
#include "waxwing/str_util.hh"

namespace waxwing {
using internal::Connection;
using internal::Router;
using internal::Socket;
using internal::concurrency::ThreadPool;

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

void concat_headers(std::string& buf, const Headers& headers) {
    for (const auto& [key, value] : headers) {
        concat_header(buf, key, value);
    }
}

std::string read_body(const Connection& conn, std::string_view initial,
                      const size_t length) {
    constexpr size_t CHUNK_SIZE = 1024;

    std::string body{initial};
    size_t bytes_read = initial.size();
    do {
        bytes_read += conn.recv(body, CHUNK_SIZE);
    } while (bytes_read < length);

    return body;
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

Result<Request, std::string> read_request(const Connection& conn) {
    constexpr size_t HEADERS_BUFFER_SIZE = 2048;  // 2 Kb

    std::string buf;
    conn.recv(buf, HEADERS_BUFFER_SIZE);

    str_util::Split line_split = str_util::split(buf, "\r\n");

    std::optional<std::pair<HttpMethod, std::string>> request_line_opt =
        and_then(line_split.next(), [](const std::string_view line) {
            return parse_request_line(line);
        });
    if (!request_line_opt) {
        return Error{"Error parsing request line"};
    }

    auto [method, target] = std::move(*request_line_opt);

    RequestBuilder builder{method, std::move(target)};
    Headers headers;

    for (std::optional<std::string_view> line = line_split.next();
         line.has_value(); line = line_split.next()) {
        if (line->empty()) {
            break;
        }

        auto [key, value] = parse_header(*line);
        headers.get(key) = value;
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
                return Error{content_length.error()};
            }

            builder.body(
                read_body(conn, line_split.remaining(), *content_length));
        } else {
            builder.body(line_split.remaining());
        }
    }

    builder.headers(std::move(headers));
    return std::move(builder).build();
}

void send_response(const Connection& conn, Response& resp) noexcept {
    std::string buf;

    buf += "HTTP/1.1 ";
    buf += format_status_code(resp.status());
    buf += "\r\n";

    Headers& headers = resp.headers();

    const std::optional<std::string_view> body_opt = resp.body();
    if (body_opt.has_value()) {
        const std::string content_length = std::to_string(body_opt->size());
        headers.insert_or_assign("Content-Length", content_length);
    }

    // push all of the headers into buf
    concat_headers(buf, headers);

    // empty line is required even if the body is empty
    buf += "\r\n";

    if (body_opt.has_value()) {
        buf += *body_opt;
    }

    conn.send(buf);
}

void handle_connection(const Router& router, Connection connection) noexcept {
    auto req_res = read_request(connection);
    if (!req_res) {
        spdlog::error("{}", req_res.error());
        return;
    }

    const Request& req = req_res.value();
    const internal::RoutingResult route =
        router.route(req.method(), req.target());

    Response resp = route.handler()(req, route.parameters());
    spdlog::info("{} {} -> {}", format_method(req.method()), req.target(),
                 format_status_code(resp.status()));
    send_response(connection, resp);
}
}  // namespace

void Server::route(const HttpMethod method, const internal::RouteTarget target,
                   const std::function<Response()>& handler) noexcept {
    route(method, target, [handler](const Request&, const PathParameters) {
        return handler();
    });
}

void Server::route(
    const HttpMethod method, const internal::RouteTarget target,
    const std::function<Response(Request const&)>& handler) noexcept {
    route(method, target, [handler](const Request& req, const PathParameters) {
        return handler(req);
    });
}

void Server::route(
    const HttpMethod method, const internal::RouteTarget target,
    const std::function<Response(const PathParameters)>& handler) noexcept {
    route(method, target,
          [handler](const Request&, const PathParameters params) {
              return handler(params);
          });
}

void Server::route(const HttpMethod method, const internal::RouteTarget target,
                   const internal::RequestHandler& handler) noexcept {
    router_.add_route(method, target, handler);
}

void Server::print_route_tree() const noexcept { router_.print_tree(); }

void Server::set_not_found_handler(internal::RequestHandler handler) {
    router_.set_not_found_handler(handler);
}

Result<void, std::string> Server::bind(const std::string_view address,
                                       const uint16_t port,
                                       const int backlog) noexcept {
    auto sock_res = Socket::create(address, port, backlog);
    if (!sock_res) {
        return Error{std::move(sock_res.error())};
    }
    socket_ = std::move(sock_res.value());

    return {};
}

void Server::serve() const noexcept {
    ThreadPool thread_pool{std::thread::hardware_concurrency() - 1};

    for (;;) {
        Connection connection = socket_.accept();
        if (connection.is_valid()) {
            thread_pool.async([this, conn = std::move(connection)]() mutable {
                handle_connection(router_, std::move(conn));
            });
        }
    }
}
}  // namespace waxwing
