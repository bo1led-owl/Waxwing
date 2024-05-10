#pragma once

#include <memory>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include "request.hh"
#include "response.hh"
#include "types.hh"

namespace waxwing::internal {
using RequestHandler =
    std::function<std::unique_ptr<Response>(Request const&, const Params)>;

class Router {
    constexpr static auto default_404_handler = [](const Request&,
                                                   const Params&) {
        return ResponseBuilder(StatusCode::NotFound).build();
    };

    struct RouteNode {
        enum class Type {
            Literal,
            ParamNonEmpty,
            ParamAny,
        };

        Type type;
        std::string_view key;
        std::unordered_map<HttpMethod, RequestHandler> handlers;
        std::vector<std::unique_ptr<RouteNode>> children;
        std::vector<uint8_t> available_path_lengths;

        RouteNode(const Type type, const std::string_view key)
            : type{type}, key{parse_key(key)} {}

        /// Parse path component key from parameter like `*name` or `:action`
        static std::string_view parse_key(std::string_view key) noexcept;
        /// Parse path component type from its string representation
        static Type parse_type(std::string_view s) noexcept;
        void print(uint8_t layer = 0, bool last = false) const noexcept;
    };

    std::unique_ptr<RouteNode> root_;
    RequestHandler not_found_handler_;

public:
    Router(RequestHandler not_found_handler = default_404_handler)
        : root_{std::make_unique<RouteNode>(Router::RouteNode::Type::Literal,
                                            "")},
          not_found_handler_{not_found_handler} {}

    /// Insert new route into the tree, returns whether the new value was
    /// inserted
    bool add_route(HttpMethod method, std::string_view target,
                   const RequestHandler& handler) noexcept;

    /// Parse given target and return corresponding request handler and parsed
    /// path parameters. If handler was not found, returns 404 hanlder
    std::pair<RequestHandler, std::vector<std::string_view>> route(
        HttpMethod method, std::string_view target) const noexcept;

    void set_not_found_handler(RequestHandler handler) noexcept;
    void print_tree() const noexcept;
};
}  // namespace waxwing::internal
