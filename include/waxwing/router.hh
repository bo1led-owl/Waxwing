#pragma once

#include <fmt/core.h>

#include <algorithm>
#include <functional>
#include <string_view>
#include <utility>
#include <vector>

#include "waxwing/http.hh"
#include "waxwing/request.hh"
#include "waxwing/response.hh"
#include "waxwing/str_split.hh"

namespace waxwing::internal {
using RequestHandler =
    std::function<Response(Request const&, const PathParameters)>;

/// Class for compile-time checks of targets
class RouteTarget {
    std::string_view target_;

    // this is bad but C++ does not provide a constexpr `isalnum` function
    static consteval bool is_legal_char(char c) noexcept {
        return ('0' <= c && c <= '9') || ('a' <= c && c <= 'z') ||
               ('A' <= c && c <= 'Z') || c == '*' || c == ':' || c == '.' ||
               c == '_' || c == '-';
    }

public:
    template <typename S>
        requires(std::constructible_from<std::string_view, S>)
    consteval RouteTarget(S&& target) : target_{std::forward<S>(target)} {
        if (!check(target_)) {
            throw std::invalid_argument("Invalid target");
        }
    }

    static consteval bool check(std::string_view target) noexcept {
        if (target.starts_with('/')) {
            target = target.substr(1);
        }

        auto split = str_util::split(target, '/');
        for (std::optional<std::string_view> component = split.next();
             component.has_value(); component = split.next()) {
            const size_t param_indicators_count =
                std::count_if(component->begin(), component->end(),
                              [](char c) { return c == '*' || c == ':'; });
            if (param_indicators_count > 1) {
                return false;
            }
            if (param_indicators_count == 1 &&
                !(component->starts_with('*') || component->starts_with(':'))) {
                return false;
            }

            const bool all_chars_are_valid = std::all_of(
                component->begin(), component->end(), is_legal_char);
            if (!all_chars_are_valid) {
                return false;
            }
        }

        return true;
    }

    constexpr operator std::string_view() const noexcept { return target_; }
};

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

        void insert_or_replace_handler(HttpMethod method,
                                       RequestHandler handler);
        Node& insert_or_get_child(Node&& child);

        std::optional<RequestHandler> find_handler(
            HttpMethod method) const noexcept;

        std::vector<std::reference_wrapper<const Node>> find_matching_children(
            std::string_view component) const noexcept;

        void print(uint8_t layer = 0, bool last = false) const noexcept;
    };

    Node root_{""};
    static void insert(Node& cur_node, HttpMethod method,
                       std::string_view target, RequestHandler handler);
    static std::optional<RoutingResult> get(
        Node const& cur_node, std::vector<std::string_view>& params,
        HttpMethod method, std::string_view target) noexcept;

public:
    RouteTree() = default;

    void insert(HttpMethod method, std::string_view target,
                RequestHandler handler) noexcept;

    std::optional<RoutingResult> get(HttpMethod method,
                                     std::string_view target) const noexcept;

    void print() const noexcept;
};

class Router final {
    constexpr static auto default_not_found_handler =
        [](const Request&, const PathParameters&) {
            return ResponseBuilder(HttpStatusCode::NotFound_404).build();
        };

    RouteTree tree_{};
    RequestHandler not_found_handler_;

public:
    Router(RequestHandler not_found_handler = default_not_found_handler)
        : not_found_handler_{not_found_handler} {}

    void add_route(HttpMethod method, std::string_view target,
                   const RequestHandler& handler) noexcept;

    /// Parse given target and return corresponding request handler and parsed
    /// path parameters. If handler was not found, returns 404 hanlder
    RoutingResult route(HttpMethod method,
                        std::string_view target) const noexcept;

    void set_not_found_handler(RequestHandler handler) noexcept;
    void print_tree() const noexcept;
};
}  // namespace waxwing::internal
