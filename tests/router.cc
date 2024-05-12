#include "waxwing/router.hh"

#include <fmt/core.h>
#include <gtest/gtest.h>

namespace waxwing {
using waxwing::internal::Router;
using waxwing::internal::RouteTree;

TEST(Router, Basic) {
    auto foo = [](const Request&, const PathParameters) {
        return ResponseBuilder{HttpStatusCode::Ok}.body("foo").build();
    };

    auto bar = [](const Request&, const PathParameters) {
        return ResponseBuilder{HttpStatusCode::Ok}.body("bar").build();
    };

    RouteTree tree;
    tree.insert(HttpMethod::Get, "/foo", foo);
    tree.insert(HttpMethod::Get, "bar", bar);

    auto req = RequestBuilder(HttpMethod::Get, "").build();

    auto result = tree.get(HttpMethod::Get, "/foo");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ((result->handler())(*req, {})->body(), "foo");

    result = tree.get(HttpMethod::Get, "/bar");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ((result->handler())(*req, {})->body(), "bar");

    result = tree.get(HttpMethod::Get, "foo");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ((result->handler())(*req, {})->body(), "foo");

    result = tree.get(HttpMethod::Get, "bar");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ((result->handler())(*req, {})->body(), "bar");

    EXPECT_FALSE(tree.get(HttpMethod::Get, "/unknown").has_value());
}

TEST(Router, PathParametersBasic) {
    auto foo_bar = [](const Request&, const PathParameters params) {
        return ResponseBuilder{HttpStatusCode::Ok}
            .body(fmt::format("{}{}", params[0], params[1]))
            .build();
    };

    RouteTree tree;
    tree.insert(HttpMethod::Get, "/:foo/*bar", foo_bar);

    auto req = RequestBuilder(HttpMethod::Get, "").build();

    auto expect_path_result =
        [&tree, &req](HttpMethod method, std::string_view path,
                      std::string_view result, size_t params_count) {
            auto opt = tree.get(method, path);
            ASSERT_TRUE(opt.has_value());
            EXPECT_EQ(opt->parameters().size(), params_count);
            EXPECT_EQ(opt->handler()(*req, opt->parameters())->body(), result);
        };

    expect_path_result(HttpMethod::Get, "/foo/", "foo", 2);
    expect_path_result(HttpMethod::Get, "foo/bar", "foobar", 2);
    expect_path_result(HttpMethod::Get, "1/2", "12", 2);

    EXPECT_FALSE(tree.get(HttpMethod::Get, "/").has_value());
    EXPECT_FALSE(tree.get(HttpMethod::Get, "").has_value());
    EXPECT_FALSE(tree.get(HttpMethod::Get, "/hello").has_value());
    EXPECT_FALSE(tree.get(HttpMethod::Get, "hello").has_value());
}

TEST(Router, PathParametersRollback) {
    auto foo_bar = [](const Request&, const PathParameters) {
        return ResponseBuilder{HttpStatusCode::Ok}.body("foo_bar").build();
    };

    auto params = [](const Request&, const PathParameters) {
        return ResponseBuilder{HttpStatusCode::Ok}.body("params").build();
    };

    RouteTree tree;
    tree.insert(HttpMethod::Get, "/foo/bar", foo_bar);
    tree.insert(HttpMethod::Get, "/:param/", params);

    auto req = RequestBuilder(HttpMethod::Get, "").build();

    auto expect_path_result =
        [&tree, &req](HttpMethod method, std::string_view path,
                      std::string_view result, size_t params_count) {
            auto opt = tree.get(method, path);
            ASSERT_TRUE(opt.has_value());
            EXPECT_EQ(opt->parameters().size(), params_count);
            EXPECT_EQ(opt->handler()(*req, opt->parameters())->body(), result);
        };

    expect_path_result(HttpMethod::Get, "/foo/", "params", 1);
    expect_path_result(HttpMethod::Get, "/foo/bar", "foo_bar", 0);

    EXPECT_FALSE(tree.get(HttpMethod::Get, "/").has_value());
    EXPECT_FALSE(tree.get(HttpMethod::Get, "").has_value());
    EXPECT_FALSE(tree.get(HttpMethod::Get, "/hello").has_value());
    EXPECT_FALSE(tree.get(HttpMethod::Get, "hello").has_value());
}

TEST(Router, RouteValidation) {
    auto foo = [](const Request&, const PathParameters) {
        return ResponseBuilder{HttpStatusCode::Ok}.build();
    };

    Router r;
    auto expect_invalid = [&r, &foo](const std::string_view target) {
        EXPECT_THROW(r.add_route(HttpMethod::Get, target, foo),
                     std::invalid_argument);
    };
    auto expect_valid = [&r, &foo](const std::string_view target) {
        EXPECT_NO_THROW(r.add_route(HttpMethod::Get, target, foo));
    };

    expect_valid("foo/bar/");
    expect_valid("/foo/bar");
    expect_valid("/:name/*action/");

    expect_invalid("/b?/");
    expect_invalid("/::foo/");
    expect_invalid("/*action*");
}

TEST(Router, RepeatingTargets) {
    auto foo = [](const Request&, const PathParameters) {
        return ResponseBuilder{HttpStatusCode::Ok}.build();
    };

    Router r;
    EXPECT_NO_THROW(r.add_route(HttpMethod::Get, "/foo", foo));
    EXPECT_THROW(r.add_route(HttpMethod::Get, "/foo", foo),
                 std::invalid_argument);
}
}  // namespace waxwing
