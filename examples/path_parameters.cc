#include <fmt/core.h>
#include <spdlog/spdlog.h>

#include <cstdint>
#include <cstdlib>
#include <string>
#include <string_view>
#include <utility>

#include "waxwing/http.hh"
#include "waxwing/response.hh"
#include "waxwing/result.hh"
#include "waxwing/server.hh"

auto name(const waxwing::PathParameters params) {
    std::string body = fmt::format("Requested user `{}`", params[0]);

    return waxwing::ResponseBuilder(waxwing::HttpStatusCode::Ok_200)
        .body(std::move(body))
        .content_type(waxwing::content_type::plaintext)
        .build();
}

auto name_action(const waxwing::PathParameters params) {
    std::string body =
        fmt::format("Requested `{}` on user `{}`", params[1], params[0]);

    return waxwing::ResponseBuilder(waxwing::HttpStatusCode::Ok_200)
        .body(std::move(body))
        .content_type(waxwing::content_type::plaintext)
        .build();
}

int main() {
    constexpr std::string_view HOST = "127.0.0.1";
    constexpr uint16_t PORT = 8080;

    waxwing::Server s{};
    s.route(waxwing::HttpMethod::Get, "/:name", name);
    s.route(waxwing::HttpMethod::Get, "/:name/*action", name_action);

    const waxwing::Result<void, std::string> bind_result = s.bind(HOST, PORT);
    if (bind_result.has_error()) {
        spdlog::error(bind_result.error());
        return EXIT_FAILURE;
    }

    spdlog::info("serving on {}:{}", HOST, PORT);
    s.serve();

    return EXIT_SUCCESS;
}
