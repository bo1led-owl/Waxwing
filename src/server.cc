#include "waxwing/server.hh"

#include <spdlog/spdlog.h>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

#include "concurrency.hh"
#include "str_split.hh"
#include "str_util.hh"
#include "waxwing/io.hh"
#include "waxwing/request.hh"
#include "waxwing/response.hh"
#include "waxwing/router.hh"
#include "waxwing/types.hh"

constexpr size_t HEADERS_BUFFER_SIZE = 2048;  // 2 Kb

namespace waxwing {
using namespace internal;

namespace {
std::pair<std::string, std::string> parse_header(const std::string_view s) {
    const size_t colon_pos = s.find(':');

    return {str_util::to_lower(str_util::trim(s.substr(0, colon_pos))),
            std::string{str_util::trim(
                s.substr(colon_pos + 1, s.size() - colon_pos - 1))}};
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

std::string read_body(const Connection& conn, std::string_view initial) {
    constexpr size_t CHUNK_SIZE = 1024;

    std::string body{initial};

    size_t bytes_read = 0;
    do {
        bytes_read = conn.recv(body, CHUNK_SIZE);
    } while (bytes_read > 0);

    return body;
}

Result<std::unique_ptr<Request>, std::string_view> read_request(
    const Connection& conn) {
    std::string buf;
    conn.recv(buf, HEADERS_BUFFER_SIZE);

    const str_util::Split line_split =
        str_util::split(std::string_view{buf.cbegin(), buf.cend()}, "\r\n");
    auto line_iter = line_split.begin();

    if (line_iter == line_split.end()) {
        return Error{"No request line"};
    }

    const auto token_split = str_util::split(*line_iter, ' ');
    auto token_iter = token_split.begin();

    auto get_tok = [&token_iter,
                    &token_split]() -> std::optional<std::string_view> {
        std::optional<std::string_view> result = std::nullopt;

        if (token_iter != token_split.end()) {
            result = *token_iter;
        }

        ++token_iter;
        return result;
    };

    std::optional<HttpMethod> method = and_then(
        get_tok(), [](const std::string_view s) { return parse_method(s); });
    std::optional<std::string_view> target = get_tok();
    const std::optional<std::string_view> http_version = get_tok();

    if (!method || !target || !http_version) {
        return Error{"Error parsing request line"};
    }
    ++line_iter;

    if (!target->starts_with('/')) {
        target = std::move(std::string("/").append(target.value()));
    }

    RequestBuilder builder{method.value(), std::move(target.value())};

    for (; line_iter != line_split.end(); ++line_iter) {
        const std::string_view line = *line_iter;
        if (line.empty()) {
            break;
        }

        auto [key, value] = parse_header(line);
        builder.header(std::move(key), std::move(value));
    };

    // only these methods are allowed to carry payload
    // https://developer.mozilla.org/en-US/docs/Web/HTTP/Methods
    if (method.value() == HttpMethod::Delete ||
        method.value() == HttpMethod::Patch ||
        method.value() == HttpMethod::Post ||
        method.value() == HttpMethod::Put) {
        builder.body(read_body(conn, line_iter.remaining()));
    }

    return builder.build();
}

void send_response(const Connection& conn, Response& resp) {
    std::string buf;

    buf += "HTTP/1.1 ";
    buf += format_status(resp.status());
    buf += "\r\n";

    Headers& headers = resp.headers();
    headers["Connection"] = "Close";

    const std::optional<Response::Body>& body_opt = resp.body();
    if (body_opt.has_value()) {
        headers["Content-Type"] = body_opt->type;

        const std::string content_length =
            std::to_string(body_opt->data.size());
        headers["Content-Length"] = content_length;
    }

    // push all of the headers into buf
    concat_headers(buf, headers);

    // empty line is required even if the body is empty
    buf += "\r\n";

    if (body_opt.has_value()) {
        buf += body_opt->data;
    }

    conn.send(buf);
}

void handle_connection(const Router& router, Connection connection) {
    auto req_res = read_request(connection);
    if (!req_res) {
        spdlog::error("{}", req_res.error());
        return;
    }

    Request& req = *req_res.value();

    auto [handler, params] = router.route(req.target(), req.method());

    std::unique_ptr<Response> resp = handler(req, params);
    spdlog::info("{} {} -> {}", format_method(req.method()), req.target(),
                 format_status(resp->status()));
    send_response(connection, *resp);
}
}  // namespace

bool Server::route(
    const HttpMethod method, const std::string_view target,
    const std::function<std::unique_ptr<Response>()>& handler) noexcept {
    const bool result = router_.add_route(
        target, method,
        [&handler](const Request&, const Params) { return handler(); });

    if (!result) {
        spdlog::warn(
            "handler for `{}` on `{}` was already present, the new one is "
            "ignored",
            format_method(method), target);
    }
    return result;
}

bool Server::route(
    const HttpMethod method, const std::string_view target,
    const std::function<std::unique_ptr<Response>(Request const&)>&
        handler) noexcept {
    const bool result = router_.add_route(
        target, method,
        [&handler](const Request& req, const Params) { return handler(req); });

    if (!result) {
        spdlog::warn(
            "handler for `{}` on `{}` was already present, the new one is "
            "ignored",
            format_method(method), target);
    }
    return result;
}

bool Server::route(const HttpMethod method, const std::string_view target,
                   const std::function<std::unique_ptr<Response>(const Params)>&
                       handler) noexcept {
    const bool result = router_.add_route(
        target, method, [&handler](const Request&, const Params params) {
            return handler(params);
        });

    if (!result) {
        spdlog::warn(
            "handler for `{}` on `{}` was already present, the new one is "
            "ignored",
            format_method(method), target);
    }
    return result;
}

bool Server::route(const HttpMethod method, const std::string_view target,
                   const std::function<std::unique_ptr<Response>(
                       Request const&, const Params)>& handler) noexcept {
    const bool result = router_.add_route(target, method, handler);

    if (!result) {
        spdlog::warn(
            "handler for `{}` on `{}` was already present, the new one is "
            "ignored",
            format_method(method), target);
    }
    return result;
}

void Server::print_route_tree() const noexcept {
    router_.print_tree();
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
    concurrency::ThreadPool thrd_pool;

    for (;;) {
        Connection connection = socket_.accept();
        if (connection.is_valid()) {
            thrd_pool.async([this, conn = std::move(connection)]() mutable {
                handle_connection(router_, std::move(conn));
            });
        }
    }
}
}  // namespace waxwing
