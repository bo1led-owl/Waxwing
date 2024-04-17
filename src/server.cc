#include "server.hh"

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include <charconv>
#include <expected>

#include "file_descriptor.hh"
#include "str_util.hh"

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

std::optional<Request> read_request(const Connection& conn) {
    std::string buf;
    conn.recv(buf, HEADERS_BUFFER_SIZE);

    str_util::SplitIterator line_iter = str_util::split(buf, "\r\n");

    // request line
    const std::optional<std::string_view> request_line = line_iter.next();
    if (!request_line.has_value()) {
        return std::nullopt;
    }

    str_util::SplitIterator token_iter =
        str_util::split(request_line.value(), ' ');

    std::optional<Method> method = token_iter.next().and_then(
        [](const std::string_view s) { return parse_method(s); });

    std::optional<std::string_view> target = token_iter.next();

    if (!method || !target || !token_iter.next()) {
        return std::nullopt;
    }

    Headers headers;

    line_iter.for_each([&headers, &line_iter](const std::string_view line) {
        if (line.empty()) {
            // request body starts here, breaking
            line_iter.finish();
        }
        const std::pair<std::string, std::string> header = parse_header(line);
        headers.insert_or_assign(std::string{header.first},
                                 std::string{header.second});
    });

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
            return std::nullopt;
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

void Server::handle_connection(const Connection& connection) const {
    std::optional<Request> req_opt = read_request(connection);
    if (!req_opt) {
        return;
    }

    Request& req = req_opt.value();

    std::pair<RequestHandler, Params> route =
        router_.route(req.target(), req.method());

    req.set_params(route.second);
    Response resp = route.first(req);
    send_response(connection, resp);
}

void Server::print_route_tree() const {
    router_.print_tree();
}

bool Server::serve(const std::string_view address, const uint16_t port) const {
    const Socket sock{address, port};
    if (!sock.is_valid()) {
        return false;
    }

    for (;;) {
        const Connection conn = sock.accept();
        if (!conn.is_valid()) {
            continue;
        }

        handle_connection(conn);
    }

    return true;
}
}  // namespace http
