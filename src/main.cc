#include <fmt/core.h>

#include <charconv>
#include <cstdint>
#include <cstdlib>
#include <string>
#include <string_view>

#include "request.hh"
#include "response.hh"
#include "server.hh"

http::Response hello(const http::Request& req) {
    const std::string resp =
        fmt::format("Hello, {}. You're dead silent.",
                    req.header("name").value_or("anonymous"));
    return http::Response(http::StatusCode::Ok)
        .body(http::ContentType::Text, resp);
}

http::Response hello_post(const http::Request& req) {
    const std::string resp =
        fmt::format("Hello, {}. I heard you say \"{}\".",
                    req.header("name").value_or("anonymous"), req.body());

    return http::Response(http::StatusCode::Ok)
        .body(http::ContentType::Text, resp);
}

http::Response name(const http::Request& req) {
    const std::string resp = fmt::format("hi {}!", req.path_parameter("name"));

    return http::Response(http::StatusCode::Ok)
        .body(http::ContentType::Text, resp);
}

http::Response name_action(const http::Request& req) {
    const std::string resp = fmt::format("{} is {}", req.path_parameter("name"),
                                         req.path_parameter("action"));
    return http::Response(http::StatusCode::Ok)
        .body(http::ContentType::Text, resp);
}

uint64_t fact_calc(uint64_t n) {
    uint64_t res = 1;
    for (; n > 0; --n) {
        res *= n;
    }
    return res;
}

http::Response fact(const http::Request& req) {
    const std::string_view s = req.path_parameter("n");
    uint64_t n;
    std::from_chars(s.begin(), s.end(), n);
    const std::string resp = fmt::format("{}", fact_calc(n));

    return http::Response(http::StatusCode::Ok)
        .body(http::ContentType::Text, resp);
}

int main() {
    constexpr std::string_view HOST = "127.0.0.1";
    constexpr uint16_t PORT = 8080;

    http::Server s;

    s.route("/hello", http::Method::Get, hello);
    s.route("/hello", http::Method::Post, hello_post);
    s.route("/user/:name", http::Method::Get, name);
    s.route("/user/:name/*action", http::Method::Get, name_action);
    s.route("/fact/:n", http::Method::Get, fact);

    s.print_route_tree();

    if (auto result = s.serve(HOST, PORT); result.has_error()) {
        fmt::println("Error: {}", result.error());
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
