#include <stdlib.h>

#include "server.h"
#include "types.h"

void hello(http_context_t* ctx) {
    dynamic_str_t response = (dynamic_str_t){0};

    string_concat(&response, str_from_cstr("Hello, "));
    const str_t* name = headers_get(&ctx->req.headers, str_from_cstr("Name"));
    if (name) {
        string_concat(&response, *name);
    } else {
        string_concat(&response, str_from_cstr("anonymous"));
    }

    string_concat(&response, str_from_cstr(". You are dead silent"));

    ctx_respond(ctx, STATUS_OK, CONTENT_TEXT, str_from_string(&response));
    string_free(&response);
}

void hello_post(http_context_t* ctx) {
    dynamic_str_t response = (dynamic_str_t){0};

    string_concat(&response, str_from_cstr("Hello, "));
    const str_t* name = headers_get(&ctx->req.headers, str_from_cstr("Name"));
    if (name) {
        string_concat(&response, *name);
    } else {
        string_concat(&response, str_from_cstr("anonymous"));
    }

    string_concat(&response, str_from_cstr(". I heard you say `"));
    string_concat(&response, ctx->req.body);
    string_push(&response, '`');

    ctx_respond(ctx, STATUS_OK, CONTENT_TEXT, str_from_string(&response));
    string_free(&response);
}

void not_found(http_context_t* ctx) {
    str_t page = load_html("./res/404.html");
    ctx_respond(ctx, STATUS_NOT_FOUND, CONTENT_HTML, page);
    free(page.data);
}

i32 main() {
    http_router_t r;
    router_init(&r, not_found);

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
