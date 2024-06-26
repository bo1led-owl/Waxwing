#include <spdlog/spdlog.h>

#include <cstdint>
#include <cstdlib>
#include <string_view>

#include "waxwing/server.hh"

auto hello_world() {
    return waxwing::ResponseBuilder(waxwing::HttpStatusCode::Ok_200)
        .body("Hello world!")
        .content_type(waxwing::content_type::plaintext)
        .build();
}

int main() {
    constexpr std::string_view HOST = "127.0.0.1";
    constexpr uint16_t PORT = 8080;

    // spdlog::set_level(spdlog::level::off);

    waxwing::Server s{};
    s.route(waxwing::HttpMethod::Get, "/hello", hello_world);

    const waxwing::Result<void, std::string> bind_result = s.bind(HOST, PORT);
    if (bind_result.has_error()) {
        spdlog::error(bind_result.error());
        return EXIT_FAILURE;
    }

    spdlog::info("serving on {}:{}", HOST, PORT);
    s.serve();

    return EXIT_SUCCESS;
}
