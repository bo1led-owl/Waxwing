#include "waxwing/router.hh"

#include <fmt/core.h>

#include <algorithm>
#include <cassert>
#include <iostream>
#include <string_view>

namespace waxwing::internal {
// ===== RouteResult =====
RequestHandler RouteResult::handler() const noexcept {
    return handler_;
}

PathParameters RouteResult::parameters() const noexcept {
    return parameters_;
}

// ===== Router =====
void Router::set_not_found_handler(RequestHandler handler) noexcept {
    not_found_handler_ = handler;
}

void Router::add_route(HttpMethod method, std::string_view target,
                       const RequestHandler& handler) {
    tree_.insert(method, target, handler);
}

void Router::print_tree() const noexcept {
    tree_.print();
}

RouteResult Router::route(HttpMethod method,
                          std::string_view target) const noexcept {
    return tree_.get(method, target)
        .value_or(RouteResult{not_found_handler_, {}});
}

// ===== RouteTree::Node =====
RouteTree::Node::Node(std::string_view key)
    : type_{parse_key(key).first}, key_{parse_key(key).second} {}

std::pair<RouteTree::Node::Type, std::string_view> RouteTree::Node::parse_key(
    const std::string_view key) noexcept {
    if (key.empty()) {
        return {Type::Literal, key};
    }

    switch (key[0]) {
        case '*':
            return {Type::ParamAny, key.substr(1)};
        case ':':
            return {Type::ParamNonEmpty, key.substr(1)};
        default:
            return {Type::Literal, key};
    }
}

namespace {
void print_node_tree_segment(const uint8_t layer, const bool last) {
    if (layer > 0 && !last) {
        std::cout << "|";
    }

    if (layer >= 1) {
        for (int i = 1; i < layer * 2 - 2; ++i) {
            std::cout << "  ";
        }
        std::cout << "`- ";
    }
}
}  // namespace

void RouteTree::Node::print(const uint8_t layer,
                            const bool last) const noexcept {
    print_node_tree_segment(layer, last);

    if (key_.empty()) {
        std::cout << '/';
    } else {
        switch (type_) {
            case Type::Literal:
                break;
            case Type::ParamNonEmpty:
                std::cout << ':';
                break;
            case Type::ParamAny:
                std::cout << '*';
                break;
        }
        std::cout << key_;
    }
    std::cout << ' ';
    for (const auto& [method, _] : handlers_) {
        std::cout << format_method(method) << ' ';
    }
    std::cout << '\n';

    for (size_t i = 0; i < children_.size(); ++i) {
        children_[i].print(layer + 1,
                           (layer == 0 && i == children_.size() - 1) || last);
    }
}

RouteTree::Node::Type RouteTree::Node::type() const noexcept {
    return type_;
}

std::string_view RouteTree::Node::key() const noexcept {
    return key_;
}

std::strong_ordering RouteTree::Node::operator<=>(
    const Node& rhs) const noexcept {
    const int diff = static_cast<int>(type_) - static_cast<int>(rhs.type_);
    if (diff < 0) {
        return std::strong_ordering::less;
    } else if (diff == 0) {
        return std::strong_ordering::equal;
    } else {
        return std::strong_ordering::greater;
    }
}

bool RouteTree::Node::is_parameter() const noexcept {
    return type_ == Type::ParamNonEmpty || type_ == Type::ParamAny;
}

bool RouteTree::Node::matches(std::string_view key) const noexcept {
    switch (type_) {
        case Type::Literal:
            return key == key_;
        case Type::ParamNonEmpty:
            return !key.empty();
        case Type::ParamAny:
            return true;
        default:
            __builtin_unreachable();
    }
}

bool RouteTree::Node::insert_handler(HttpMethod method,
                                     RequestHandler handler) {
    if (std::find_if(
            handlers_.cbegin(), handlers_.cend(),
            [method](const std::pair<HttpMethod, RequestHandler>& hanlder) {
                return hanlder.first == method;
            }) != handlers_.end()) {
        return false;
    }

    handlers_.emplace_back(method, handler);
    return true;
}

RouteTree::Node& RouteTree::Node::insert_or_get_child(Node&& child) {
    auto iter = std::find_if(
        children_.begin(), children_.end(), [&child](const Node& node) {
            return node.key_ == child.key_ && node.type_ == child.type_;
        });

    if (iter != children_.end()) {
        return *iter;
    }

    Node& result = children_.emplace_back(std::move(child));
    return result;
}

std::optional<RequestHandler> RouteTree::Node::find_handler(
    HttpMethod method) const noexcept {
    auto iter = std::find_if(
        handlers_.cbegin(), handlers_.cend(),
        [method](const std::pair<HttpMethod, RequestHandler>& hanlder) {
            return hanlder.first == method;
        });

    if (iter == handlers_.end()) {
        return std::nullopt;
    }

    return iter->second;
}

std::vector<RouteTree::Node const*> RouteTree::Node::find_matching_children(
    std::string_view component) const noexcept {
    std::vector<Node const*> result;

    for (const auto& child : children_) {
        if (child.matches(component)) {
            result.push_back(&child);
        }
    }

    std::sort(result.begin(), result.end(),
              [](Node const* lhs, Node const* rhs) { return *lhs < *rhs; });

    return result;
}

// ===== RouteTree =====
void RouteTree::print() const noexcept {
    root_.print();
}

bool RouteTree::insert(Node& cur_node, HttpMethod method,
                       std::string_view target, RequestHandler handler) {
    const size_t slash_pos = target.find('/');
    const std::string_view component = target.substr(0, slash_pos);

    if (slash_pos == std::string_view::npos) {
        return cur_node.insert_or_get_child(Node{component})
            .insert_handler(method, handler);
    }

    target = target.substr(slash_pos + 1);
    return insert(cur_node.insert_or_get_child(Node{component}), method, target,
                  handler);
}

void RouteTree::insert(HttpMethod method, std::string_view target,
                       RequestHandler handler) {
    // leading slash is insignificant
    if (target.starts_with('/')) {
        target = target.substr(1);
    }

    if (!insert(root_, method, target, handler)) {
        throw std::invalid_argument(
            fmt::format("Handler for `{}` on `{}` was already present",
                        format_method(method), target));
    }
}

std::optional<RouteResult> RouteTree::get(
    Node const* cur_node, std::vector<std::string_view>& params,
    HttpMethod method, std::string_view target) const noexcept {
    assert(cur_node);

    const size_t slash_pos = target.find('/');
    std::string_view cur_component = target.substr(0, slash_pos);
    const bool found_slash = slash_pos != std::string_view::npos;

    if (!found_slash) {
        for (const Node* child :
             cur_node->find_matching_children(cur_component)) {
            assert(child);

            std::optional<RequestHandler> result = child->find_handler(method);
            if (result.has_value()) {
                params.push_back(cur_component);
                return RouteResult{*result, std::move(params)};
            }
        }

        return std::nullopt;
    }

    auto matching_children = cur_node->find_matching_children(cur_component);

    bool pushed_parameter = false;
    target = target.substr(slash_pos + 1);  // prepare target for next iteration
    for (const Node* child : matching_children) {
        assert(cur_node);

        // sorting guarantees that literals come first
        if (child->is_parameter() && !pushed_parameter) {
            params.push_back(cur_component);
            pushed_parameter = true;
        }

        std::optional<RouteResult> result = get(child, params, method, target);
        if (result.has_value()) {
            return result;
        }
    }

    if (cur_node->is_parameter()) {
        params.pop_back();
    }
    return std::nullopt;
}

std::optional<RouteResult> RouteTree::get(
    HttpMethod method, std::string_view target) const noexcept {
    // leading slash is insignificant
    if (target.starts_with('/')) {
        target = target.substr(1);
    }

    std::vector<std::string_view> params;
    return get(&root_, params, method, target);
}
}  // namespace waxwing::internal
