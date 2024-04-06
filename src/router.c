#include "router.h"

#include <stdlib.h>

enum {
    ROUTER_BUCKETS_COUNT = 64,
};

static void default_not_found_handler(http_context_t* ctx) {
    ctx_respond(ctx, STATUS_NOT_FOUND, CONTENT_NONE, str_from_cstr(""));
}

void router_init(http_router_t* router, http_req_handler_t not_found_handler) {
    *router = (http_router_t){
        .routes = calloc(ROUTER_BUCKETS_COUNT, sizeof(vector_t)),
        .not_found_handler =
            (not_found_handler) ? not_found_handler : default_not_found_handler,
    };
}

void router_free(http_router_t* router) {
    if (!router)
        return;

    for (usize i = 0; i < ROUTER_BUCKETS_COUNT; ++i) {
        vector_free(&router->routes[i]);
    }
    free(router->routes);
}

bool router_add_route(http_router_t* router, const http_method_t method,
                      const char* target, http_req_handler_t handler) {
    const http_route_t route = {.method = method,
                                .target = str_from_cstr((char*)target),
                                .handler = handler};
    const u64 hash = str_hash(route.target) % ROUTER_BUCKETS_COUNT;
    for (usize i = 0; i < router->routes[hash].size; ++i) {
        const http_route_t* cur_route =
            (http_route_t*)router->routes[hash].items + i;
        if (str_compare(route.target, cur_route->target) == 0 &&
            route.method == cur_route->method) {
            return true;
        }
    }

    // didn't find the route in the loop above, inserting
    vector_push(&router->routes[hash], sizeof(http_route_t), &route, 1);
    return false;
}

http_req_handler_t router_get_route(const http_router_t* router,
                                    const http_method_t method,
                                    const str_t target) {
    const u64 hash = str_hash(target) % ROUTER_BUCKETS_COUNT;
    for (usize i = 0; i < router->routes[hash].size; ++i) {
        const http_route_t* cur_route =
            &((http_route_t*)router->routes[hash].items)[i];
        if (str_compare(target, cur_route->target) == 0 &&
            cur_route->method == method) {
            return cur_route->handler;
        }
    }

    // didn't find the route in the loop above
    return nullptr;
}
