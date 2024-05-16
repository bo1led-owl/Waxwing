#include <fmt/core.h>
#include <spdlog/spdlog.h>

#include <cstdint>
#include <cstdlib>
#include <string_view>

#include "waxwing/server.hh"

auto read_header(const waxwing::Request& req) {
    std::optional<std::string_view> header = req.header("Key");

    if (header.has_value()) {
        std::string body = fmt::format("OK key `{}`", *header);
        return waxwing::ResponseBuilder(waxwing::HttpStatusCode::Ok)
            .body(std::move(body))
            .content_type(waxwing::content_type::plaintext())
            .build();
    } else {
        return waxwing::ResponseBuilder(waxwing::HttpStatusCode::BadRequest)
            .body("No key provided")
            .content_type(waxwing::content_type::plaintext())
            .build();
    }
}

auto write_header() {
    return waxwing::ResponseBuilder(waxwing::HttpStatusCode::Ok)
        .header("Key", "1234")
        .build();
}

int main() {
    constexpr std::string_view HOST = "127.0.0.1";
    constexpr uint16_t PORT = 8080;

    waxwing::Server s{};
    s.route(waxwing::HttpMethod::Get, "/auth", read_header);
    s.route(waxwing::HttpMethod::Get, "/get_key", write_header);

    const waxwing::Result<void, std::string> bind_result = s.bind(HOST, PORT);
    if (bind_result.has_error()) {
        spdlog::error(bind_result.error());
        return EXIT_FAILURE;
    }

    spdlog::info("serving on {}:{}", HOST, PORT);
    s.serve();

    return EXIT_SUCCESS;
}
