#include <stdlib.h>

#include "server.h"
#include "types.h"

void greet(const str_t* name, dynamic_str_t* buf) {
    string_concat(buf, str_from_cstr("Hello, "));
    if (name) {
        string_concat(buf, *name);
    } else {
        string_concat(buf, str_from_cstr("anonymous"));
    }
    string_concat(buf, str_from_cstr(". "));
}

void hello(http_context_t* ctx) {
    dynamic_str_t response = (dynamic_str_t){0};

    const str_t* name = ctx_get_request_header(ctx, str_from_cstr("Name"));
    greet(name, &response);
    string_concat(&response, str_from_cstr("You are dead silent"));

    ctx_respond_msg(ctx, STATUS_OK, CONTENT_TEXT, str_from_string(&response));
    string_free(&response);
}

void hello_post(http_context_t* ctx) {
    dynamic_str_t response = (dynamic_str_t){0};

    const str_t* name = ctx_get_request_header(ctx, str_from_cstr("Name"));
    greet(name, &response);

    string_concat(&response, str_from_cstr("I heard you say `"));
    string_concat(&response, ctx->req.body);
    string_push(&response, '`');

    ctx_respond_msg(ctx, STATUS_OK, CONTENT_TEXT, str_from_string(&response));
    string_free(&response);
}

void foo(http_context_t* ctx) {
    ctx_respond_empty(ctx, STATUS_I_AM_A_TEAPOT);
}

void name(http_context_t* ctx) {
    dynamic_str_t response = (dynamic_str_t){0};
    const str_t name = *ctx_get_parameter(ctx, str_from_cstr("name"));

    string_concat(&response, str_from_cstr("hi "));
    string_concat(&response, name);
    ctx_respond_msg(ctx, STATUS_OK, CONTENT_TEXT, str_from_string(&response));
    string_free(&response);
}

void name_action(http_context_t* ctx) {
    dynamic_str_t response = (dynamic_str_t){0};
    const str_t name = *ctx_get_parameter(ctx, str_from_cstr("name"));
    const str_t action = *ctx_get_parameter(ctx, str_from_cstr("action"));

    string_concat(&response, name);
    string_concat(&response, str_from_cstr(" is "));
    string_concat(&response, action);
    ctx_respond_msg(ctx, STATUS_OK, CONTENT_TEXT, str_from_string(&response));
    string_free(&response);
}

i32 main() {
    http_server_t s;
    if (!server_init(&s, 8122)) {
        server_free(&s);
        return EXIT_FAILURE;
    }

    add_route(&s, METHOD_GET, "/hello", hello);
    add_route(&s, METHOD_POST, "/hello", hello_post);
    add_route(&s, METHOD_GET, "/foo", foo);
    add_route(&s, METHOD_GET, "/user/:name/*action", name_action);
    add_route(&s, METHOD_GET, "/user/:name", name);

    router_print_tree(&s.router);

    server_loop(&s);

    server_free(&s);
    return EXIT_SUCCESS;
}
