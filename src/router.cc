#include "server/router.hh"

#include <iostream>
#include <ranges>
#include <string>

#include "str_split.hh"

namespace http {
namespace internal {
std::string_view Router::RouteNode::get_path_parameter_key(
    const std::string_view key) {
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
    const std::string_view s) {
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

void Router::RouteNode::print(const int layer) const {
    for (int i = 0; i < layer * 2; ++i) {
        std::cout << ' ';
    }
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

    for (const auto& child : children) {
        child->print(layer + 1);
    }
}

void Router::set_not_found_handler(const RequestHandler& handler) {
    not_found_handler_ = handler;
}

void Router::print_tree() const {
    root_->print();
}

void Router::add_route(const std::string_view target, const Method method,
                       const RequestHandler& handler) {
    RouteNode* cur_node = root_.get();
    str_util::Split split = str_util::split(target, '/');
    auto iter = split.begin();
    ++iter;

    for (std::string_view component :
         std::ranges::subrange(iter, split.end())) {
        const RouteNode::Type type = RouteNode::parse_type(component);
        component = RouteNode::get_path_parameter_key(component);

        for (const auto& child : cur_node->children) {
            if (child->key == component && child->type == type) {
                cur_node = child.get();
                goto skip;
            }
        }

        cur_node->children.emplace_back(
            std::make_unique<RouteNode>(type, component));
        cur_node = cur_node->children.back().get();
    skip:;
    };

    cur_node->handlers[method] = handler;
}

std::pair<RequestHandler, Params> Router::route(const std::string_view target,
                                                const Method method) const {
    Params params;

    RouteNode const* cur_node = root_.get();
    str_util::Split split = str_util::split(target, '/');
    auto iter = split.begin();
    ++iter;

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
            return {not_found_handler_, params};
        }

    found_literal:;
    };

    const auto handler_it = cur_node->handlers.find(method);
    if (handler_it == cur_node->handlers.end()) {
        return {not_found_handler_, params};
    }

    return {handler_it->second, params};
}
}  // namespace internal
}  // namespace http
