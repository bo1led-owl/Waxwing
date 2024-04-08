#include "server.h"

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

enum {
    HEADERS_BUFFER_SIZE = 2048,  // 2 Kb
    MAX_CONNECTIONS = 8,
};

static void print_method(http_method_t method) {
    switch (method) {
        case METHOD_UNKNOWN:
            printf("UNKNOWN");
            break;
        case METHOD_GET:
            printf("GET");
            break;
        case METHOD_POST:
            printf("POST");
            break;
    }
}

static void print_req(const http_request_t* req) {
    print_method(req->method);
    putchar(' ');
    str_print(req->target);
    // putchar(' ');
    // str_print(req->version);
    putchar('\n');
}

static http_method_t parse_method(const str_t method) {
    if (str_compare(method, str_from_cstr("GET")) == 0) {
        return METHOD_GET;
    }
    if (str_compare(method, str_from_cstr("POST")) == 0) {
        return METHOD_POST;
    }
    return METHOD_UNKNOWN;
}

static str_pair_t parse_header(const str_t input) {
    usize colon_pos = 0;
    for (; colon_pos < input.len; ++colon_pos) {
        if (input.data[colon_pos] == ':')
            break;
    }

    return (str_pair_t){
        .key = str_trim((str_t){.data = input.data, .len = colon_pos}),
        .value = str_trim((str_t){.data = input.data + colon_pos + 1,
                                  .len = input.len - colon_pos - 1})};
}

static bool parse_request(const str_t req, http_request_t* result) {
    str_split_iter_t line_iter = str_split(req, str_from_cstr("\r\n"));

    // request line
    const str_t request_line = split_next(&line_iter);
    if (request_line.data == nullptr) {
        return false;
    }

    str_split_iter_t token_iter = str_split(request_line, str_from_cstr(" "));

    result->method = parse_method(split_next(&token_iter));
    result->target = split_next(&token_iter);

    if (result->method == METHOD_UNKNOWN || !result->target.data ||
        !split_next(&token_iter).data) {
        return false;
    }

    split_for_each(line_iter, line) {
        if (line.len == 0) {
            break;
        }
        const str_pair_t header = parse_header(line);
        str_hashmap_insert(&result->headers, header.key, header.value);
    }

    result->body = str_trim(line_iter.source);
    return true;
}

void add_route(http_server_t* server, const http_method_t method,
               const char* target, const http_req_handler_t handler) {
    router_add_route(&server->router, method, target, handler);
}

static void ctx_free(http_context_t* ctx) {
    str_hashmap_free(&ctx->req.headers);
    str_hashmap_free(&ctx->req.params);
    *ctx = (http_context_t){0};
}

void server_loop(http_server_t* server) {
    str_t buf = (str_t){.data = malloc(HEADERS_BUFFER_SIZE * sizeof(char))};

    http_context_t ctx = (http_context_t){0};

    // TODO: exit condition?
    struct sockaddr_in clientaddr;
    socklen_t clientaddr_len = sizeof(clientaddr);
    for (;;) {
        ctx.writer.conn_fd = accept(
            server->sock_fd, (struct sockaddr*)&clientaddr, &clientaddr_len);

        if (ctx.writer.conn_fd == -1) {
            perror("Error accepting connection");
            continue;
        }

        buf.len = recv(ctx.writer.conn_fd, buf.data,
                       HEADERS_BUFFER_SIZE * sizeof(char), 0);

        ctx.req = (http_request_t){.headers = ctx.req.headers,
                                   .params = ctx.req.params};
        if (!parse_request(buf, &ctx.req)) {
            fprintf(stderr, "Error parsing request\n");
        }

        print_req(&ctx.req);

        const http_req_handler_t handler = router_get_route(
            &server->router, ctx.req.method, ctx.req.target, &ctx.req.params);
        if (handler != nullptr) {
            handler(&ctx);
        } else {
            server->router.not_found_handler(&ctx);
        }

        vector_free(&ctx.writer.headers);
        str_hashmap_clear(&ctx.req.headers);
        str_hashmap_clear(&ctx.req.params);
        close(ctx.writer.conn_fd);
    }

    ctx_free(&ctx);
    free(buf.data);
}

bool server_init(http_server_t* server, const i32 port) {
    if (!server)
        return false;

    router_init(&server->router, nullptr);

    server->sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server->sock_fd == -1) {
        perror("Error creating socket");
        return false;
    }

    const int option = 1;
    setsockopt(server->sock_fd, SOL_SOCKET, SO_REUSEADDR, &option,
               sizeof(option));

    struct sockaddr_in addr = (struct sockaddr_in){
        .sin_family = AF_INET,
        .sin_port = htons(port),
        .sin_addr = (struct in_addr){.s_addr = htonl(INADDR_ANY)}};
    socklen_t len = sizeof(addr);

    if (bind(server->sock_fd, (struct sockaddr*)&addr, len) == -1) {
        perror("Error binding socket");
        return false;
    }

    if (listen(server->sock_fd, MAX_CONNECTIONS) == -1) {
        perror("Error starting to listen");
        return false;
    }

    return true;
}

void server_free(http_server_t* server) {
    router_free(&server->router);
    close(server->sock_fd);
}
