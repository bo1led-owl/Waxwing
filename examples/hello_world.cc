#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string_view>

#include "waxwing/server.hh"

auto hello_world(const waxwing::Request&) {
    return waxwing::ResponseBuilder(waxwing::StatusCode::Ok)
        .body(waxwing::ContentType::Text, "Hello world!")
        .build();
}

int main() {
    constexpr std::string_view HOST = "127.0.0.1";
    constexpr uint16_t PORT = 8080;

    waxwing::Server s{};
    s.route(waxwing::Method::Get, "/hello", hello_world);

    const waxwing::Result<void, std::string_view> bind_result = s.bind(HOST, PORT);
    if (bind_result.has_error()) {
        std::cerr << "Error: " << bind_result.error();
        return EXIT_FAILURE;
    }

    s.serve();

    return EXIT_SUCCESS;
}
