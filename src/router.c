#include "router.h"

#include <stdio.h>
#include <stdlib.h>

typedef struct handler_entry {
    http_method_t method;
    http_req_handler_t func;
} handler_entry_t;

static void default_not_found_handler(http_context_t* ctx) {
    ctx_respond_empty(ctx, STATUS_NOT_FOUND);
}

void router_init(http_router_t* router, http_req_handler_t not_found_handler) {
    *router = (http_router_t){
        .not_found_handler =
            (not_found_handler) ? not_found_handler : default_not_found_handler,
        .root = malloc(sizeof(http_route_node_t)),
    };

    *router->root = (http_route_node_t){.key = (str_t){}};
}

static http_route_node_type_t node_get_type(const str_t key) {
    if (key.len == 0)
        return ROUTE_NODE_LITERAL;

    switch (key.data[0]) {
        case '*':
            return ROUTE_NODE_PARAMETER_ANY;
        case ':':
            return ROUTE_NODE_PARAMETER_NON_EMPTY;
        default:
            return ROUTE_NODE_LITERAL;
    }
}

static void node_print(http_route_node_t* node, i32 layer) {
    if (!node)
        return;

    for (i32 i = 0; i < layer * 2; ++i) {
        putchar(' ');
    }
    if (node->key.len == 0) {
        putchar('/');
    } else {
        str_print(node->key);
    }
    putchar(' ');
    for (usize i = 0; i < node->handlers.size; ++i) {
        switch (((handler_entry_t*)node->handlers.items)[i].method) {
            case METHOD_UNKNOWN:
                printf("UNKNOWN ");
                break;
            case METHOD_GET:
                printf("GET ");
                break;
            case METHOD_POST:
                printf("POST ");
                break;
        }
    }
    putchar('\n');

    for (usize i = 0; i < node->children.size; ++i) {
        node_print(((http_route_node_t**)node->children.items)[i], layer + 1);
    }
}

void router_print_tree(const http_router_t* r) {
    node_print(r->root, 0);
}

static void node_free(http_route_node_t* node) {
    if (!node)
        return;

    vector_free(&node->handlers);
    for (usize i = 0; i < node->children.size; ++i) {
        // will this explode the stack? probably yes
        node_free(((http_route_node_t**)node->children.items)[i]);
    }
}

void router_free(http_router_t* router) {
    if (!router)
        return;

    node_free(router->root);
}

http_req_handler_t node_find_method(const http_route_node_t* node,
                                    const http_method_t method) {
    if (!node)
        return nullptr;

    const handler_entry_t* handlers = node->handlers.items;
    for (usize i = 0; i < node->handlers.size; ++i) {
        if (handlers[i].method == method) {
            return handlers[i].func;
        }
    }

    return nullptr;
}

static void node_insert_handler(http_route_node_t* node,
                                const http_method_t method,
                                const http_req_handler_t func) {
    if (!node)
        return;

    const handler_entry_t handler =
        (handler_entry_t){.method = method, .func = func};

    vector_push(&node->handlers, sizeof(handler), &handler, 1);
}

static str_t node_strip_key(const str_t key) {
    if (key.len == 0)
        return key;

    switch (key.data[0]) {
        case '*':
        case ':':
            return (str_t){.data = key.data + 1, .len = key.len - 1};
        default:
            return key;
    }
}

static void node_insert_child(http_route_node_t* node, const str_t key,
                              const http_route_node_type_t type) {
    if (!node)
        return;

    http_route_node_t* new_node = malloc(sizeof(http_route_node_t));
    *new_node = (http_route_node_t){.key = node_strip_key(key), .type = type};
    vector_push(&node->children, sizeof(http_route_node_t*), &new_node, 1);
}

bool router_add_route(http_router_t* router, const http_method_t method,
                      const char* target, http_req_handler_t handler) {
    http_route_node_t* cur_node = router->root;
    const str_t target_str = str_from_cstr((char*)target);
    str_split_iter_t path_iter = str_split(target_str, str_from_cstr("/"));

    split_next(&path_iter);  // skip first, empty part of the path
    split_for_each(path_iter, path_component) {
        const http_route_node_type_t component_type =
            node_get_type(path_component);
        path_component = node_strip_key(path_component);

        http_route_node_t** children = cur_node->children.items;
        for (usize i = 0; i < cur_node->children.size; ++i) {
            if (str_compare(children[i]->key, path_component) == 0) {
                cur_node = children[i];
                goto found;  // skip inserting new node, go to the next
                             // component
            }
        }

        // didn't found a node in the loop above, have to insert it
        node_insert_child(cur_node, path_component, component_type);
        cur_node = ((http_route_node_t**)
                        cur_node->children.items)[cur_node->children.size - 1];

    found:
    }

    const handler_entry_t* handlers = cur_node->handlers.items;
    for (usize i = 0; i < cur_node->handlers.size; ++i) {
        if (handlers[i].method == method) {
            return true;
        }
    }

    node_insert_handler(cur_node, method, handler);
    return false;
}

http_req_handler_t router_get_route(const http_router_t* router,
                                    const http_method_t method,
                                    const str_t target, str_hashmap_t* params) {
    http_route_node_t const* cur_node = router->root;
    str_split_iter_t path_iter = str_split(target, str_from_cstr("/"));

    // skip first, empty part of the path
    split_next(&path_iter);
    split_for_each(path_iter, path_component) {
        const http_route_node_t* parameter_node = nullptr;

        const http_route_node_t** children = cur_node->children.items;
        for (usize i = 0; i < cur_node->children.size; ++i) {
            if (children[i]->type == ROUTE_NODE_PARAMETER_ANY &&
                (path_component.len == 0 || parameter_node == nullptr)) {
                parameter_node = children[i];
            } else if (children[i]->type == ROUTE_NODE_PARAMETER_NON_EMPTY &&
                       (path_component.len != 0)) {
                parameter_node = children[i];
            } else if (children[i]->type == ROUTE_NODE_LITERAL &&
                       str_compare(children[i]->key, path_component) == 0) {
                cur_node = children[i];
                goto found_literal;
            }
        }

        if (parameter_node != nullptr) {
            str_hashmap_insert(params, parameter_node->key, path_component);
            cur_node = parameter_node;
        } else {
            return nullptr;
        }

    found_literal:
    }

    return node_find_method(cur_node, method);
}
