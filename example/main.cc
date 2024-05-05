#include <fmt/core.h>

#include <charconv>
#include <cstdint>
#include <cstdlib>
#include <string>
#include <string_view>

#include "server/server.hh"

http::Response hello(const http::Request& req) {
    const std::string_view name = req.header("name").value_or("anonymous");
    const std::string resp =
        fmt::format("Hello, {}. You're dead silent.", name);
    return http::Response(http::StatusCode::Ok)
        .body(http::ContentType::Text, resp);
}

http::Response hello_post(const http::Request& req) {
    const std::string_view name = req.header("name").value_or("anonymous");
    const std::string resp =
        fmt::format("Hello, {}. I heard you say \"{}\".", name, req.body());

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


    const http::Result<void, std::string_view> serve_result = s.serve(HOST, PORT);
    if (serve_result.has_error()) {
        fmt::println("Error: {}", serve_result.error());
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
