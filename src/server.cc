#include "server.hh"

#include <charconv>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <utility>

#include "concurrency.hh"
#include "file_descriptor.hh"
#include "request.hh"
#include "response.hh"
#include "result.hh"
#include "router.hh"
#include "str_split.hh"
#include "str_util.hh"
#include "types.hh"

namespace http {
namespace {
constexpr size_t HEADERS_BUFFER_SIZE = 2048;  // 2 Kb

std::optional<Method> parse_method(const std::string_view s) {
    if (s == "GET") {
        return Method::Get;
    }
    if (s == "POST") {
        return Method::Post;
    }
    return std::nullopt;
}

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

Result<Request, std::string_view> read_request(const Connection& conn) {
    std::string buf;
    conn.recv(buf, HEADERS_BUFFER_SIZE);

    const auto line_split = str_util::split(buf, "\r\n");
    auto line_iter = line_split.begin();

    if (line_iter == line_split.end()) {
        return Error<std::string_view>{"No request line"};
    }

    const auto token_split = str_util::split(*line_iter, ' ');
    auto token_iter = token_split.begin();

    auto get_tok = [&token_iter,
                    &token_split]() -> std::optional<std::string_view> {
        std::optional<std::string_view> result = std::nullopt;

        if (token_iter != token_split.end()) {
            result = *token_iter;
        }

        token_iter++;
        return result;
    };

    const std::optional<Method> method =
        get_tok().and_then([](std::string_view s) { return parse_method(s); });
    const std::optional<std::string_view> target = get_tok();
    const std::optional<std::string_view> http_version = get_tok();

    if (!method || !target || !http_version) {
        return Error{"Error parsing request line"};
    }

    Headers headers;

    for (const std::string_view line :
         std::ranges::subrange(line_iter, line_split.end())) {
        if (line.empty()) {
            // request body starts here, breaking
            break;
        }
        const std::pair<std::string, std::string> header = parse_header(line);
        headers.insert_or_assign(std::string{header.first},
                                 std::string{header.second});
    };

    std::string body{line_iter.remaining()};

    const auto content_length_it = headers.find("content-length");
    if (content_length_it != headers.end()) {
        // content-length header is present
        const std::string_view raw_content_length = content_length_it->second;

        size_t content_length;
        const auto [_, ec] =
            std::from_chars(raw_content_length.begin(),
                            raw_content_length.end(), content_length);

        if (ec != std::errc{}) {
            return Error{"Error parsing `Content-Length`"};
        }

        body.reserve(body.size() + content_length);
        while (content_length > body.size()) {
            conn.recv(body, content_length - body.size());
        }
    }

    return Request{method.value(), std::string{target.value()}, headers, body};
}

void send_response(const Connection& conn, Response& resp) {
    std::string buf;

    buf += "HTTP/1.1 ";
    buf += format_status(resp.get_status());
    buf += "\r\n";

    resp.header("Connection", "Close");

    const std::optional<Response::Body>& body_opt = resp.get_body();
    if (body_opt.has_value()) {
        resp.header("Content-Type",
                    std::string{format_content_type(body_opt->type)});

        const std::string content_length =
            std::to_string(body_opt->data.size());
        resp.header("Content-Length", content_length);
    }

    // push all of the headers into buf
    concat_headers(buf, resp.get_headers());

    // empty line is required even if the body is empty
    buf += "\r\n";

    if (body_opt.has_value()) {
        buf += body_opt->data;
    }

    conn.send(buf);
}
}  // namespace

void Server::route(const std::string_view target, const Method method,
                   const RequestHandler& handler) {
    router_.add_route(target, method, handler);
}

Result<void, std::string_view> Server::handle_connection(
    Connection connection) const {
    auto req_res = read_request(connection);
    if (!req_res) {
        return Error{req_res.error()};
    }

    Request& req = req_res.value();

    std::pair<RequestHandler, Params> route =
        router_.route(req.target(), req.method());

    req.set_params(route.second);
    Response resp = route.first(req);
    send_response(connection, resp);

    return {};
}

void Server::print_route_tree() const {
    router_.print_tree();
}

Result<void, std::string_view> Server::serve(const std::string_view address,
                                             const uint16_t port) const {
    auto sock_res = Socket::create(address, port);
    if (!sock_res) {
        return Error{sock_res.error()};
    }
    Socket sock = std::move(sock_res.value());

    concurrency::ThreadPool thrd_pool;

    for (;;) {
        Connection conn = sock.accept();
        if (conn.is_valid()) {
            thrd_pool.async([this, _conn = std::move(conn)] mutable {
                handle_connection(std::move(_conn));
            });
        }
    }

    return {};
}
}  // namespace http
