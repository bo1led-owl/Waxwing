#include "waxwing/router.hh"

#include <iostream>
#include <ranges>
#include <string>

#include "str_split.hh"

namespace waxwing {
namespace internal {
std::string_view Router::RouteNode::parse_key(
    const std::string_view key) noexcept {
    if (key.empty()) {
        return key;
    }

    switch (key[0]) {
        case '*':
        case ':':
            return key.substr(1);
        default:
            return key;
    }
}

Router::RouteNode::Type Router::RouteNode::parse_type(
    const std::string_view s) noexcept {
    if (s.empty()) {
        return Type::Literal;
    }

    switch (s[0]) {
        case '*':
            return Type::ParamAny;
        case ':':
            return Type::ParamNonEmpty;
        default:
            return Type::Literal;
    }
}

void print_node_tree_segment(const uint8_t layer, const bool last) {
    if (!last) {
        if (layer > 0) {
            std::cout << '|';

            if (layer == 1) {
                std::cout << '-';
            } else {
                for (int i = 1; i < layer * 2 - 2; ++i) {
                    std::cout << ' ';
                }
                std::cout << "`-";
            }
            std::cout << ' ';
        }
    } else {
        if (layer > 1) {
            for (int i = 1; i < layer * 2 - 2; ++i) {
                std::cout << ' ';
            }
        }
        std::cout << "`- ";
    }
}

void Router::RouteNode::print(const uint8_t layer,
                              const bool last) const noexcept {
    print_node_tree_segment(layer, last);

    if (key.empty()) {
        std::cout << '/';
    } else {
        switch (type) {
            case Type::Literal:
                break;
            case Type::ParamNonEmpty:
                std::cout << ':';
                break;
            case Type::ParamAny:
                std::cout << '*';
                break;
        }
        std::cout << key;
    }
    std::cout << ' ';
    for (const auto& [method, _] : handlers) {
        std::cout << format_method(method) << ' ';
    }
    std::cout << '\n';

    for (size_t i = 0; i < children.size(); ++i) {
        children[i]->print(layer + 1,
                           (layer == 0 && i == children.size() - 1) || last);
    }
}

void Router::set_not_found_handler(const RequestHandler& handler) noexcept {
    not_found_handler_ = handler;
}

void Router::print_tree() const noexcept {
    root_->print();
}

bool Router::add_route(const std::string_view target, const Method method,
                       const RequestHandler& handler) noexcept {
    RouteNode* cur_node = root_.get();
    str_util::Split split = str_util::split(target, '/');
    auto iter = split.begin();
    if (target.starts_with('/')) {
        ++iter;
    }

    for (std::string_view component :
         std::ranges::subrange(iter, split.end())) {
        const RouteNode::Type type = RouteNode::parse_type(component);

        component = RouteNode::parse_key(component);

        for (const auto& child : cur_node->children) {
            if (child->key == component && child->type == type) {
                cur_node = child.get();
                goto skip;
            }
        }

        cur_node =
            cur_node->children
                .emplace_back(std::make_unique<RouteNode>(type, component))
                .get();
    skip:;
    };

    auto insertion_it = cur_node->handlers.find(method);

    if (insertion_it == cur_node->handlers.end()) {
        cur_node->handlers.emplace(method, handler);
        return true;
    } else {
        return false;
    }
}

std::pair<RequestHandler, Params> Router::route(
    const std::string_view target, const Method method) const noexcept {
    Params params;

    RouteNode const* cur_node = root_.get();
    str_util::Split split = str_util::split(target, '/');
    auto iter = split.begin();
    if (target.starts_with('/')) {
        ++iter;
    }

    for (const std::string_view component :
         std::ranges::subrange(iter, split.end())) {
        RouteNode const* parameter_node = nullptr;

        for (const auto& child : cur_node->children) {
            if ((child->type == RouteNode::Type::ParamAny &&
                 (component.empty() || parameter_node == nullptr)) ||
                (child->type == RouteNode::Type::ParamNonEmpty &&
                 !component.empty())) {
                parameter_node = child.get();
            } else if (child->type == RouteNode::Type::Literal &&
                       child->key == component) {
                cur_node = child.get();
                goto found_literal;
            }
        }

        if (parameter_node) {
            params.insert(
                {std::string{parameter_node->key}, std::string{component}});
            cur_node = parameter_node;
        } else {
            return {not_found_handler_, std::move(params)};
        }

    found_literal:;
    };

    const auto handler_it = cur_node->handlers.find(method);
    if (handler_it == cur_node->handlers.end()) {
        return {not_found_handler_, std::move(params)};
    }

    return {handler_it->second, std::move(params)};
}
}  // namespace internal
}  // namespace waxwing
