#pragma once

#include <functional>
#include <memory>

#include "request.hh"
#include "response.hh"
#include "types.hh"

namespace http {
using RequestHandler = std::function<Response(Request const&)>;

class Router {
    constexpr static auto default_404_handler = [](const Request&) {
        return Response(StatusCode::NotFound);
    };

    struct RouteNode {
        enum class Type {
            Literal,
            ParamNonEmpty,
            ParamAny,
        };

        Type type;
        std::string_view key;
        std::unordered_map<Method, RequestHandler> handlers;
        std::vector<std::unique_ptr<RouteNode>> children;

        RouteNode(const Type type, const std::string_view key)
            : type{type}, key{get_path_parameter_key(key)} {}

        static std::string_view get_path_parameter_key(
            const std::string_view key);
        static Type parse_type(const std::string_view s);
        void print(const int layer = 0) const;
    };

    std::unique_ptr<RouteNode> root_;
    RequestHandler not_found_handler_;

public:
    Router(const RequestHandler& not_found_handler = default_404_handler)
        : root_{std::make_unique<RouteNode>(Router::RouteNode::Type::Literal,
                                            "")},
          not_found_handler_{not_found_handler} {}

    void add_route(const std::string_view target, const Method method,
                   const RequestHandler handler);
    std::pair<RequestHandler, Params> route(const std::string_view target,
                                            const Method method) const;
    void set_not_found_handler(const RequestHandler& handler);
    void print_tree() const;
};
}  // namespace http
