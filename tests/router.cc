#include "waxwing/router.hh"

#include <fmt/core.h>
#include <gtest/gtest.h>

namespace waxwing {
using waxwing::internal::Router;

TEST(Router, Basic) {
    auto foo = [](const Request&, const Params) {
        return ResponseBuilder{StatusCode::Ok}
            .body(ContentType::Text, "foo")
            .build();
    };

    auto bar = [](const Request&, const Params) {
        return ResponseBuilder{StatusCode::Ok}
            .body(ContentType::Text, "bar")
            .build();
    };

    Router router;
    router.add_route(HttpMethod::Get, "/foo", foo);
    router.add_route(HttpMethod::Get, "bar", bar);

    auto req = RequestBuilder(HttpMethod::Get, "").build();

    EXPECT_EQ(
        (router.route(HttpMethod::Get, "/foo").first)(*req, {})->body()->data,
        "foo");
    EXPECT_EQ(
        (router.route(HttpMethod::Get, "/bar").first)(*req, {})->body()->data,
        "bar");

    EXPECT_EQ(
        (router.route(HttpMethod::Get, "foo").first)(*req, {})->body()->data,
        "foo");
    EXPECT_EQ(
        (router.route(HttpMethod::Get, "bar").first)(*req, {})->body()->data,
        "bar");

    EXPECT_EQ(
        (router.route(HttpMethod::Get, "/unknown").first)(*req, {})->status(),
        StatusCode::NotFound);
}

TEST(Router, PathParameters) {
    auto foo_bar = [](const Request&, const Params params) {
        return ResponseBuilder{StatusCode::Ok}
            .body(ContentType::Text, fmt::format("{}{}", params[0], params[1]))
            .build();
    };

    Router router;
    router.add_route(HttpMethod::Get, "/:foo/*bar", foo_bar);

    auto req = RequestBuilder(HttpMethod::Get, "").build();

    auto expect_path_result = [&router, &req](std::string_view path,
                                              HttpMethod method,
                                              std::string_view result) {
        auto [handler, params] = router.route(method, path);
        EXPECT_EQ(handler(*req, params)->body()->data, result);
    };

    expect_path_result("/foo/", HttpMethod::Get, "foo");
    expect_path_result("foo/bar", HttpMethod::Get, "foobar");
    expect_path_result("1/2", HttpMethod::Get, "12");

    EXPECT_EQ((router.route(HttpMethod::Get, "/").first)(*req, {})->status(),
              StatusCode::NotFound);
    EXPECT_EQ((router.route(HttpMethod::Get, "").first)(*req, {})->status(),
              StatusCode::NotFound);
    EXPECT_EQ(
        (router.route(HttpMethod::Get, "/hello").first)(*req, {})->status(),
        StatusCode::NotFound);
    EXPECT_EQ(
        (router.route(HttpMethod::Get, "hello").first)(*req, {})->status(),
        StatusCode::NotFound);
}
}  // namespace waxwing
