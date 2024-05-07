#include <charconv>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string_view>
#include <system_error>

#include "waxwing/server.hh"

class TemperatureMeter {
    float temp_;

public:
    TemperatureMeter(float inital_temp) : temp_{inital_temp} {}

    auto stat_get(const waxwing::Request&) const {
        return waxwing::ResponseBuilder(waxwing::StatusCode::Ok)
            .body(waxwing::ContentType::Text, std::to_string(temp_))
            .build();
    }

    auto stat_post(const waxwing::Request& req) {
        auto [_, error_code] =
            std::from_chars(req.body().cbegin(), req.body().cend(), temp_);

        if (error_code == std::errc()) {
            return waxwing::ResponseBuilder(waxwing::StatusCode::Ok).build();
        }

        return waxwing::ResponseBuilder(waxwing::StatusCode::BadRequest)
            .body(waxwing::ContentType::Text,
                  "Error parsing temperature: " +
                      std::make_error_code(error_code).message())
            .build();
    }
};

int main() {
    constexpr std::string_view HOST = "127.0.0.1";
    constexpr uint16_t PORT = 8080;

    waxwing::Server s{};

    TemperatureMeter meter{25.0f};
    s.route(
        waxwing::Method::Get, "/stat",
        [&meter](const waxwing::Request& req) { return meter.stat_get(req); });
    s.route(
        waxwing::Method::Post, "/stat",
        [&meter](const waxwing::Request& req) { return meter.stat_post(req); });

    const waxwing::Result<void, std::string> bind_result = s.bind(HOST, PORT);
    if (bind_result.has_error()) {
        std::cerr << "Error: " << bind_result.error();
        return EXIT_FAILURE;
    }

    s.serve();

    return EXIT_SUCCESS;
}
