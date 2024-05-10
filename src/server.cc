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

#include "str_split.hh"
#include "str_util.hh"
#include "thread_pool.hh"
#include "waxwing/io.hh"
#include "waxwing/request.hh"
#include "waxwing/response.hh"
#include "waxwing/router.hh"
#include "waxwing/types.hh"

namespace waxwing {
using namespace internal;

namespace {
std::pair<std::string, std::string> parse_header(const std::string_view line) {
    const size_t colon_pos = line.find(':');

    return {str_util::to_lower(str_util::trim(line.substr(0, colon_pos))),
            std::string{str_util::trim(
                line.substr(colon_pos + 1, line.size() - colon_pos - 1))}};
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
    const auto token_split = str_util::split(line, ' ');
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
    std::optional<std::string> target =
        map(get_tok(), [](std::string_view s) { return std::string{s}; });
    const std::optional<std::string_view> http_version = get_tok();

    if (!method || !target || !http_version) {
        return std::nullopt;
    }
    return {{method.value(), std::move(*target)}};
}

Result<size_t, std::string> parse_content_length(const Headers& headers) {
    size_t content_length;
    std::string_view raw_content_length =
        headers.find("content-length")->second;

    const std::errc error_code =
        std::from_chars(raw_content_length.cbegin(), raw_content_length.cend(),
                        content_length)
            .ec;
    if (error_code != std::errc{}) {
        return Error{fmt::format("couldn't parse `Content-Length`: {}",
                                 std::make_error_code(error_code).message())};
    }
    return content_length;
}

Result<std::unique_ptr<Request>, std::string> read_request(
    const Connection& conn) {
    constexpr size_t HEADERS_BUFFER_SIZE = 2048;  // 2 Kb

    std::string buf;
    conn.recv(buf, HEADERS_BUFFER_SIZE);

    const str_util::Split line_split = str_util::split(buf, "\r\n");
    auto line_iter = line_split.begin();

    std::optional<std::pair<HttpMethod, std::string>> request_line_opt =
        parse_request_line(*line_iter);
    if (!request_line_opt) {
        return Error{"Error parsing request line"};
    }

    auto [method, target] = std::move(*request_line_opt);
    ++line_iter;

    if (!target.starts_with('/')) {
        target = std::move(std::string("/").append(target));
    }

    RequestBuilder builder{method, std::move(target)};
    Headers headers;

    for (; line_iter != line_split.end(); ++line_iter) {
        const std::string_view line = *line_iter;
        if (line.empty()) {
            break;
        }

        auto [key, value] = parse_header(line);
        headers.emplace(std::move(key), std::move(value));
    };

    // only these methods are allowed to carry payload
    // https://developer.mozilla.org/en-US/docs/Web/HTTP/Methods
    if (method == HttpMethod::Delete || method == HttpMethod::Patch ||
        method == HttpMethod::Post || method == HttpMethod::Put) {
        if (headers.contains("content-length")) {
            Result<size_t, std::string> content_length =
                parse_content_length(headers);
            if (!content_length) {
                return Error{content_length.error()};
            }

            builder.body(
                read_body(conn, line_iter.remaining(), *content_length));
        } else {
            builder.body(line_iter.remaining());
        }
    }

    builder.headers(std::move(headers));
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
        [handler](const Request&, const Params) { return handler(); });

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
        [handler](const Request& req, const Params) { return handler(req); });

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
        target, method, [handler](const Request&, const Params params) {
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
