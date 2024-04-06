#pragma once

#include <sys/socket.h>

#include "router.h"
#include "types.h"

typedef struct http_server {
    http_router_t* router;
    i32 sock_fd;
} http_server_t;

bool server_init(http_server_t* server, http_router_t* router, const i32 port);
void server_free(http_server_t* server);
void server_loop(http_server_t* server);
