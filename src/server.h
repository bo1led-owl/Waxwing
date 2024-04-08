#pragma once

#include <sys/socket.h>

#include "context.h"
#include "router.h"
#include "types.h"

typedef struct http_server {
    http_router_t router;
    i32 sock_fd;
} http_server_t;

/*
Map function `handler` to the request with `method` on path `target`. Path is
expected to be a static string.
*/
void add_route(http_server_t* server, const http_method_t method,
               const char* target, const http_req_handler_t handler);

/*
Initialize a server.

Returns whether the initalization was successful
*/
bool server_init(http_server_t* server, const i32 port);

/*
Free all data corresponding to the server and its router
*/
void server_free(http_server_t* server);

/*
Run the main loop of the server
*/
void server_loop(http_server_t* server);
