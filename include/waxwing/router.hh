#pragma once

#include <memory>
#include <string_view>
#include <utility>
#include <vector>

#include "request.hh"
#include "response.hh"
#include "types.hh"

namespace waxwing::internal {
using RequestHandler = std::function<std::unique_ptr<Response>(
    Request const&, const PathParameters)>;

class RoutingResult final {
    RequestHandler handler_;
    std::vector<std::string_view> parameters_;

public:
    RoutingResult(RequestHandler handler,
                  std::vector<std::string_view>&& params)
        : handler_{handler}, parameters_{std::move(params)} {}

    RequestHandler handler() const noexcept;
    PathParameters parameters() const noexcept;
};

class RouteTree final {
    class Node final {
    public:
        enum class Type {
            Literal = 0,
            ParamNonEmpty = 1,
            ParamAny = 2,
        };

    private:
        Type type_;
        std::string_view key_;
        std::vector<Node> children_;
        std::vector<std::pair<HttpMethod, RequestHandler>> handlers_;

        static Type parse_type(std::string_view key) noexcept;
        static std::string_view parse_key(std::string_view key) noexcept;

    public:
        Node(std::string_view key);
        std::strong_ordering operator<=>(const Node&) const noexcept;

        Type type() const noexcept;
        std::string_view key() const noexcept;

        bool matches(std::string_view key) const noexcept;
        bool is_parameter() const noexcept;

        /// Returns true if handler wasn't present
        bool insert_handler(HttpMethod method, RequestHandler handler);

        Node& insert_or_get_child(Node&& child);

        std::optional<RequestHandler> find_handler(
            HttpMethod method) const noexcept;

        std::vector<std::reference_wrapper<const Node>> find_matching_children(
            std::string_view component) const noexcept;

        void print(uint8_t layer = 0, bool last = false) const noexcept;
    };

    Node root_{""};
    static bool insert(Node& cur_node, HttpMethod method,
                       std::string_view target, RequestHandler handler);
    static std::optional<RoutingResult> get(
        Node const& cur_node, std::vector<std::string_view>& params,
        HttpMethod method, std::string_view target) noexcept;

public:
    RouteTree() = default;

    void insert(HttpMethod method, std::string_view target,
                RequestHandler handler);

    std::optional<RoutingResult> get(HttpMethod method,
                                     std::string_view target) const noexcept;

    void print() const noexcept;
};

class Router final {
    constexpr static auto default_not_found_handler =
        [](const Request&, const PathParameters&) {
            return ResponseBuilder(HttpStatusCode::NotFound).build();
        };

    RouteTree tree_{};
    RequestHandler not_found_handler_;

public:
    Router(RequestHandler not_found_handler = default_not_found_handler)
        : not_found_handler_{not_found_handler} {}

    void add_route(HttpMethod method, std::string_view target,
                   const RequestHandler& handler);

    /// Parse given target and return corresponding request handler and parsed
    /// path parameters. If handler was not found, returns 404 hanlder
    RoutingResult route(HttpMethod method,
                        std::string_view target) const noexcept;

    void set_not_found_handler(RequestHandler handler) noexcept;
    void print_tree() const noexcept;
};
}  // namespace waxwing::internal
