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

void server_loop(http_server_t* server) {
    str_t buf = (str_t){.data = malloc(HEADERS_BUFFER_SIZE * sizeof(char))};

    http_context_t ctx = (http_context_t){0};

    http_headers_t response_headers;
    headers_init(&response_headers);
    headers_init(&ctx.req.headers);

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

        if (!parse_request(buf, &ctx.req)) {
            fprintf(stderr, "Error parsing request\n");
        }
        print_req(&ctx.req);

        ctx.writer.headers = &response_headers;
        const http_req_handler_t handler =
            router_get_route(server->router, ctx.req.method, ctx.req.target);
        if (handler != nullptr) {
            handler(&ctx);
        } else {
            server->router->not_found_handler(&ctx);
        }

        headers_clear(&response_headers);
        headers_clear(&ctx.req.headers);
        close(ctx.writer.conn_fd);
    }

    headers_free(&response_headers);
    free(buf.data);
}

bool server_init(http_server_t* server, http_router_t* router, const i32 port) {
    server->router = router;
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
    router_free(server->router);
    close(server->sock_fd);
}
