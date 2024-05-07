#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string_view>

#include "waxwing/server.hh"

auto read_header(const waxwing::Request& req) {
    std::optional<std::string_view> header = req.header("Key");

    std::stringstream ss;
    if (header.has_value()) {
        ss << "OK key " << std::quoted(header.value());
        return waxwing::ResponseBuilder(waxwing::StatusCode::Ok)
            .body(waxwing::ContentType::Text, ss.str())
            .build();
    } else {
        ss << "No key provided";
        return waxwing::ResponseBuilder(waxwing::StatusCode::BadRequest)
            .body(waxwing::ContentType::Text, ss.str())
            .build();
    }
}

auto write_header(const waxwing::Request&) {
    return waxwing::ResponseBuilder(waxwing::StatusCode::Ok)
        .header("Key", "1234")
        .build();
}

int main() {
    constexpr std::string_view HOST = "127.0.0.1";
    constexpr uint16_t PORT = 8080;

    waxwing::Server s{};
    s.route(waxwing::Method::Get, "/auth", read_header);
    s.route(waxwing::Method::Get, "/get_key", write_header);

    const waxwing::Result<void, std::string> bind_result =
        s.bind(HOST, PORT);
    if (bind_result.has_error()) {
        std::cerr << "Error: " << bind_result.error();
        return EXIT_FAILURE;
    }

    s.serve();

    return EXIT_SUCCESS;
}
