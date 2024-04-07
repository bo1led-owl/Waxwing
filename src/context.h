#pragma once

#include "headers.h"
#include "str.h"
#include "vector.h"

typedef enum http_method {
    METHOD_UNKNOWN,
    METHOD_GET,
    METHOD_POST,
} http_method_t;

typedef enum http_content_type {
    CONTENT_TEXT,
    CONTENT_HTML,
    CONTENT_JSON,
    CONTENT_XML,
    CONTENT_CSS,
    CONTENT_JAVASCRIPT,
} http_content_type_t;

typedef enum http_status_code {
    STATUS_OK = 200,
    STATUS_BAD_REQUEST = 400,
    STATUS_FORBIDDEN = 403,
    STATUS_NOT_FOUND = 404,
    STATUS_METHOD_NOT_ALLOWED = 405,
    STATUS_INTERNAL_SERVER_ERROR = 500,
} http_status_code_t;

typedef struct http_request {
    http_method_t method;
    str_t target;
    http_headers_t headers;
    str_t body;
} http_request_t;

typedef struct http_response_writer {
    vector_t headers;
    i32 conn_fd;
} http_response_writer_t;

typedef struct http_context {
    http_request_t req;
    http_response_writer_t writer;
} http_context_t;

bool parse_request(const str_t input, http_request_t* result);

void print_method(const enum http_method method);
void print_req(const http_request_t* req);

str_t load_html(const char* path);
str_t* ctx_get_request_header(const http_context_t* ctx, const str_t key);
void ctx_set_response_header(http_context_t* ctx, const str_t key,
                             const str_t value);
void ctx_respond_status(http_context_t* ctx, const http_status_code_t status);
void ctx_respond_msg(http_context_t* ctx, const http_status_code_t status,
                     const http_content_type_t content_type, const str_t body);
void ctx_free(http_context_t* ctx);
