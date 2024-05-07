#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <sstream>
#include <string_view>

#include "waxwing/server.hh"

auto name(const waxwing::Request& req) {
    std::stringstream ss;
    ss << "Requested user " << std::quoted(req.path_parameter("name"));

    return waxwing::ResponseBuilder(waxwing::StatusCode::Ok)
        .body(waxwing::ContentType::Text, ss.str())
        .build();
}

auto name_action(const waxwing::Request& req) {
    std::stringstream ss;
    ss << "Requested " << std::quoted(req.path_parameter("action")) << " on "
       << std::quoted(req.path_parameter("name"));

    return waxwing::ResponseBuilder(waxwing::StatusCode::Ok)
        .body(waxwing::ContentType::Text, ss.str())
        .build();
}

int main() {
    constexpr std::string_view HOST = "127.0.0.1";
    constexpr uint16_t PORT = 8080;

    waxwing::Server s{};
    s.route(waxwing::Method::Get, "/:name", name);
    s.route(waxwing::Method::Get, "/:name/*action", name_action);

    const waxwing::Result<void, std::string> bind_result =
        s.bind(HOST, PORT);
    if (bind_result.has_error()) {
        spdlog::error("Error: {}", bind_result.error());
        return EXIT_FAILURE;
    }

    spdlog::info("serving on {}:{}", HOST, PORT);
    s.serve();

    return EXIT_SUCCESS;
}
