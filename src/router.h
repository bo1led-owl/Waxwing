#pragma once

#include "context.h"
#include "vector.h"

typedef void (*http_req_handler_t)(http_context_t*);

typedef enum http_route_node_type {
    ROUTE_NODE_LITERAL = 0,
    ROUTE_NODE_PARAMETER_NON_EMPTY,
    ROUTE_NODE_PARAMETER_ANY,
} http_route_node_type_t;
typedef struct http_route_node {
    http_route_node_type_t type;
    str_t key;
    vector_t handlers;
    vector_t children;
} http_route_node_t;

typedef struct http_router {
    http_route_node_t* root;
    http_req_handler_t not_found_handler;
} http_router_t;

/// 404 handler is defaulted if nullptr is provided
void router_init(http_router_t* router, http_req_handler_t not_found_handler);
void router_free(http_router_t* router);

/// inserts a route if it wasn't present, returns whether the route was already
/// present
bool router_add_route(http_router_t* router, const enum http_method method,
                      const char* target, const http_req_handler_t handler);

http_req_handler_t router_get_route(const http_router_t* router,
                                    const http_method_t method,
                                    const str_t target, str_hashmap_t* params);

void router_print_tree(const http_router_t* r);
