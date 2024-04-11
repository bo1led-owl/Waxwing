#pragma once

#include "str.h"
#include "str_hashmap.h"

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
    STATUS_I_AM_A_TEAPOT = 418,
    STATUS_INTERNAL_SERVER_ERROR = 500,
} http_status_code_t;

typedef struct http_request {
    http_method_t method;
    str_t target;
    str_hashmap_t headers;
    str_hashmap_t params;
    str_t body;
} http_request_t;

typedef struct http_response_writer {
    str_hashmap_t headers;
    i32 conn_fd;
} http_response_writer_t;

typedef struct http_context {
    http_request_t req;
    http_response_writer_t writer;
} http_context_t;

/*
Load an HTML page from file on `path`. Returns a `str` with pointer to a
dynamically allocated memory with contents of the file. If loading
failed, pointer is set to null.
*/
dynamic_str_t load_html(const char* path);

/*
Get a const pointer to value of the header by its key. Returns null pointer
if the header is not found
*/
str_t const* ctx_get_request_header(const http_context_t* ctx, const str_t key);

/*
Get a const pointer to value of the path parameter by its key. Returns null
pointer if the header is not found
*/
str_t const* ctx_get_parameter(const http_context_t* ctx, const str_t key);

/*
Set the value of header with `key` to `value`. Does not overwrite its previous
value.
*/
void ctx_set_response_header(http_context_t* ctx, const str_t key,
                             const str_t value);

/*
Send a response with an empty body and only a status. For sending responses with
a payload, use `ctx_respond_msg`
*/
void ctx_respond_empty(http_context_t* ctx, const http_status_code_t status);

/*
Send a response with a given body and status. For sending responses with
no payload, use `ctx_respond_empty`
*/
void ctx_respond_msg(http_context_t* ctx, const http_status_code_t status,
                     const http_content_type_t content_type, const str_t body);
