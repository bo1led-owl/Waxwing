#include <stdlib.h>

#include "router.h"
#include "server.h"
#include "types.h"

void greet(const http_context_t* ctx, dynamic_str_t* buf) {
    string_concat(buf, str_from_cstr("Hello, "));
    const str_t* name = ctx_get_request_header(ctx, str_from_cstr("Name"));
    if (name) {
        string_concat(buf, *name);
    } else {
        string_concat(buf, str_from_cstr("anonymous"));
    }
    string_concat(buf, str_from_cstr(". "));
}

void hello(http_context_t* ctx) {
    dynamic_str_t response = (dynamic_str_t){0};

    greet(ctx, &response);
    string_concat(&response, str_from_cstr("You are dead silent"));

    ctx_respond_msg(ctx, STATUS_OK, CONTENT_TEXT, str_from_string(&response));
    string_free(&response);
}

void hello_post(http_context_t* ctx) {
    dynamic_str_t response = (dynamic_str_t){0};

    greet(ctx, &response);

    string_concat(&response, str_from_cstr("I heard you say `"));
    string_concat(&response, ctx->req.body);
    string_push(&response, '`');

    ctx_respond_msg(ctx, STATUS_OK, CONTENT_TEXT, str_from_string(&response));
    string_free(&response);
}

i32 main() {
    http_router_t r;
    router_init(&r, nullptr);

    router_add_route(&r, METHOD_GET, "/hello", hello);
    router_add_route(&r, METHOD_POST, "/hello", hello_post);

    http_server_t s;
    if (!server_init(&s, &r, 8122)) {
        server_free(&s);
        return EXIT_FAILURE;
    }

    server_loop(&s);
    server_free(&s);
    return EXIT_SUCCESS;
}
