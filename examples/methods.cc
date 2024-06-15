#include <spdlog/spdlog.h>

#include <cstdint>
#include <cstdlib>
#include <string_view>

#include "waxwing/server.hh"

auto echo_get() {
    return waxwing::ResponseBuilder{waxwing::HttpStatusCode::Ok_200}.build();
}

auto echo_post(const waxwing::Request& req) {
    return waxwing::ResponseBuilder{waxwing::HttpStatusCode::Ok_200}
        .body(req.body())
        .content_type(req.header("content-type")
                          .value_or(waxwing::content_type::plaintext))
        .build();
}

int main() {
    constexpr std::string_view HOST = "127.0.0.1";
    constexpr uint16_t PORT = 8080;

    waxwing::Server s{};

    s.route(waxwing::HttpMethod::Get, "/echo", echo_get);
    s.route(waxwing::HttpMethod::Post, "/echo", echo_post);

    const waxwing::Result<void, std::string> bind_result = s.bind(HOST, PORT);
    if (bind_result.has_error()) {
        spdlog::error(bind_result.error());
        return EXIT_FAILURE;
    }

    spdlog::info("serving on {}:{}", HOST, PORT);
    s.serve();

    return EXIT_SUCCESS;
}
