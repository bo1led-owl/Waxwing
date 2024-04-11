#include "server.h"

#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
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

static str_pair_t parse_header(str_t input) {
    usize colon_pos = 0;
    for (; colon_pos < input.len; ++colon_pos) {
        if (input.data[colon_pos] == ':')
            break;
    }

    str_pair_t result = (str_pair_t){
        .key = str_trim((str_t){.data = input.data, .len = colon_pos}),
        .value = str_trim((str_t){.data = input.data + colon_pos + 1,
                                  .len = input.len - colon_pos - 1})};
    str_to_lower(result.value);
    str_to_lower(result.key);
    return result;
}

static bool read_request(const i32 conn_fd, const str_t headers,
                         http_request_t* result) {
    str_split_iter_t line_iter = str_split(headers, str_from_cstr("\r\n"));

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
            // request body starts here, breaking
            break;
        }
        const str_pair_t header = parse_header(line);
        // str_print(header.key);
        // putchar(' ');
        // str_print(header.value);
        // putchar('\n');
        str_hashmap_insert(&result->headers, header.key, header.value);
    }
    split_next(&line_iter);  // skip empty line before request body

    // reading body of the request
    dynamic_str_t body = (dynamic_str_t){0};
    const str_t* raw_content_length =
        str_hashmap_get(&result->headers, str_from_cstr("content-length"));
    if (raw_content_length) {
        // content-length header is present
        char* endptr;
        const usize content_length =
            strtoull(raw_content_length->data, &endptr, 10);
        string_reserve(&body, content_length);

        if (endptr != raw_content_length->data + raw_content_length->len) {
            return false;
        }

        memcpy(body.data, line_iter.source.data, line_iter.source.len);
        body.size = line_iter.source.len;
        if (content_length > body.size) {
            body.size += recv(conn_fd, body.data + body.size,
                              content_length - body.size, 0);
        }
    } else {
        // content-length header is not present, request body is either empty or
        // small
        if (line_iter.source.len > 0) {
            string_reserve(&body, line_iter.source.len);
            memcpy(body.data, line_iter.source.data, line_iter.source.len);
            body.size = line_iter.source.len;
        }
    }

    result->body = str_from_string(&body);
    return true;
}

void add_route(http_server_t* server, const http_method_t method,
               const char* target, const http_req_handler_t handler) {
    router_add_route(&server->router, method, target, handler);
}

void set_404_handler(http_server_t* server, const http_req_handler_t handler) {
    server->router.not_found_handler = handler;
}

static void ctx_free(http_context_t* ctx) {
    free(ctx->req.body.data);
    str_hashmap_free(&ctx->req.headers);
    str_hashmap_free(&ctx->req.params);
    str_hashmap_free(&ctx->writer.headers);
}

static void handle_connection(i32 conn_fd, const http_router_t* router) {
    http_context_t ctx = (http_context_t){
        .writer = (http_response_writer_t){.conn_fd = conn_fd}};

    str_t buf = (str_t){.data = malloc(HEADERS_BUFFER_SIZE * sizeof(char))};
    buf.len = recv(conn_fd, buf.data, HEADERS_BUFFER_SIZE * sizeof(char), 0);
    if (!read_request(conn_fd, buf, &ctx.req)) {
        fprintf(stderr, "Error reading request\n");
    }

    print_req(&ctx.req);

    const http_req_handler_t handler = router_get_route(
        router, ctx.req.method, ctx.req.target, &ctx.req.params);
    if (handler != nullptr) {
        handler(&ctx);
    } else {
        router->not_found_handler(&ctx);
    }

    close(ctx.writer.conn_fd);
    ctx_free(&ctx);
    free(buf.data);
}

void server_loop(http_server_t* server) {
    // TODO: exit condition?
    struct sockaddr_in clientaddr;
    socklen_t clientaddr_len = sizeof(clientaddr);
    for (;;) {
        const i32 conn_fd = accept(
            server->sock_fd, (struct sockaddr*)&clientaddr, &clientaddr_len);

        if (conn_fd == -1) {
            perror("Error accepting connection");
            continue;
        }

        handle_connection(conn_fd, &server->router);
    }
}

bool server_init(http_server_t* server, const i32 port) {
    if (!server)
        return false;

    router_init(&server->router, nullptr);

    server->sock_fd = socket(PF_INET, SOCK_STREAM, 0);
    if (server->sock_fd == -1) {
        perror("Error creating socket");
        return false;
    }

    const int option = 1;
    setsockopt(server->sock_fd, SOL_SOCKET, SO_REUSEADDR, &option,
               sizeof(option));
    // fcntl(server->sock_fd, F_SETFL, O_NONBLOCK);

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
