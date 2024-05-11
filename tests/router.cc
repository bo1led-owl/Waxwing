#include "waxwing/router.hh"

#include <fmt/core.h>
#include <gtest/gtest.h>

namespace waxwing {
using waxwing::internal::RouteTree;

TEST(Router, Basic) {
    auto foo = [](const Request&, const PathParameters) {
        return ResponseBuilder{StatusCode::Ok}
            .body(ContentType::Text, "foo")
            .build();
    };

    auto bar = [](const Request&, const PathParameters) {
        return ResponseBuilder{StatusCode::Ok}
            .body(ContentType::Text, "bar")
            .build();
    };

    RouteTree tree;
    tree.insert(HttpMethod::Get, "/foo", foo);
    tree.insert(HttpMethod::Get, "bar", bar);

    auto req = RequestBuilder(HttpMethod::Get, "").build();

    auto result = tree.get(HttpMethod::Get, "/foo");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ((result->handler())(*req, {})->body()->data, "foo");

    result = tree.get(HttpMethod::Get, "/bar");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ((result->handler())(*req, {})->body()->data, "bar");

    result = tree.get(HttpMethod::Get, "foo");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ((result->handler())(*req, {})->body()->data, "foo");

    result = tree.get(HttpMethod::Get, "bar");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ((result->handler())(*req, {})->body()->data, "bar");

    EXPECT_FALSE(tree.get(HttpMethod::Get, "/unknown").has_value());
}

TEST(Router, PathParametersBasic) {
    auto foo_bar = [](const Request&, const PathParameters params) {
        return ResponseBuilder{StatusCode::Ok}
            .body(ContentType::Text, fmt::format("{}{}", params[0], params[1]))
            .build();
    };

    RouteTree tree;
    tree.insert(HttpMethod::Get, "/:foo/*bar", foo_bar);

    auto req = RequestBuilder(HttpMethod::Get, "").build();

    auto expect_path_result = [&tree, &req](std::string_view path,
                                            HttpMethod method,
                                            std::string_view result) {
        auto opt = tree.get(method, path);
        ASSERT_TRUE(opt.has_value());
        EXPECT_EQ(opt->handler()(*req, opt->parameters())->body()->data,
                  result);
    };

    expect_path_result("/foo/", HttpMethod::Get, "foo");
    expect_path_result("foo/bar", HttpMethod::Get, "foobar");
    expect_path_result("1/2", HttpMethod::Get, "12");

    EXPECT_FALSE(tree.get(HttpMethod::Get, "/").has_value());
    EXPECT_FALSE(tree.get(HttpMethod::Get, "").has_value());
    EXPECT_FALSE(tree.get(HttpMethod::Get, "/hello").has_value());
    EXPECT_FALSE(tree.get(HttpMethod::Get, "hello").has_value());
}

TEST(Router, PathParametersRollback) {
    auto foo = [](const Request&, const PathParameters) {
        return ResponseBuilder{StatusCode::Ok}
            .body(ContentType::Text, fmt::format("foo"))
            .build();
    };

    auto foo_bar = [](const Request&, const PathParameters) {
        return ResponseBuilder{StatusCode::Ok}
            .body(ContentType::Text, fmt::format("params"))
            .build();
    };

    RouteTree tree;
    tree.insert(HttpMethod::Get, "/foo/bar", foo);
    tree.insert(HttpMethod::Get, "/:foo/", foo_bar);

    auto req = RequestBuilder(HttpMethod::Get, "").build();

    auto expect_path_result = [&tree, &req](std::string_view path,
                                            HttpMethod method,
                                            std::string_view result) {
        auto opt = tree.get(method, path);
        ASSERT_TRUE(opt.has_value());
        EXPECT_EQ(opt->handler()(*req, opt->parameters())->body()->data,
                  result);
    };

    expect_path_result("/foo/", HttpMethod::Get, "params");
    expect_path_result("/foo/bar", HttpMethod::Get, "foo");

    EXPECT_FALSE(tree.get(HttpMethod::Get, "/").has_value());
    EXPECT_FALSE(tree.get(HttpMethod::Get, "").has_value());
    EXPECT_FALSE(tree.get(HttpMethod::Get, "/hello").has_value());
    EXPECT_FALSE(tree.get(HttpMethod::Get, "hello").has_value());
}
}  // namespace waxwing
