#include "router.hh"

#include <iostream>

#include "str_util.hh"

namespace http {
std::string_view Router::RouteNode::get_path_parameter_key(
    const std::string_view key) {
    if (key.empty())
        return key;

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
    if (s.empty())
        return Type::Literal;

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
    if (key.size() == 0) {
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
        std::cout << method << ' ';
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
                       const RequestHandler handler) {
    RouteNode* cur_node = root_.get();
    str_util::SplitIterator iter = str_util::split(target, '/');
    iter.next();
    iter.for_each([&cur_node](std::string_view component) {
        const RouteNode::Type type = RouteNode::parse_type(component);
        component = RouteNode::get_path_parameter_key(component);

        for (const auto& child : cur_node->children) {
            if (child->key == component && child->type == type) {
                cur_node = child.get();
                return;
            }
        }

        cur_node->children.emplace_back(
            std::make_unique<RouteNode>(type, component));
        cur_node = cur_node->children.back().get();
    });

    cur_node->handlers[method] = handler;
}

std::pair<RequestHandler, Params> Router::route(const std::string_view target,
                                                const Method method) const {
    Params params;

    bool breaked = false;

    RouteNode const* cur_node = root_.get();
    str_util::SplitIterator iter = str_util::split(target, '/');
    iter.next();
    iter.for_each([&cur_node, &params, &iter,
                   &breaked](const std::string_view component) {
        RouteNode const* parameter_node = nullptr;

        for (const auto& child : cur_node->children) {
            if (child->type == RouteNode::Type::ParamAny &&
                (component.empty() || parameter_node == nullptr)) {
                parameter_node = child.get();
            } else if (child->type == RouteNode::Type::ParamNonEmpty &&
                       !component.empty()) {
                parameter_node = child.get();
            } else if (child->type == RouteNode::Type::Literal &&
                       child->key == component) {
                cur_node = child.get();
                return;
            }
        }

        if (parameter_node) {
            params.insert(
                {std::string{parameter_node->key}, std::string{component}});
            cur_node = parameter_node;
        } else {
            breaked = true;
            iter.finish();
        }
    });

    if (breaked) {
        return {not_found_handler_, params};
    }

    const auto handler_it = cur_node->handlers.find(method);
    if (handler_it == cur_node->handlers.end()) {
        return {not_found_handler_, params};
    }

    return {handler_it->second, params};
}
}  // namespace http
