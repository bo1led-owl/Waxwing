#pragma once

#include "context.h"
#include "vector.h"

/// Function type for request handling
typedef void (*http_req_handler_t)(http_context_t*);

typedef struct http_route {
    enum http_method method;
    str_t target;
    http_req_handler_t handler;
} http_route_t;

typedef struct http_router {
    vector_t* routes;
    http_req_handler_t not_found_handler;
    http_req_handler_t method_not_allowed;
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
                                    const str_t target);
