#include "context.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>

enum {
    HEADERS_BUCKETS_COUNT = 64,
};

static str_t format_content_type(const http_content_type_t type) {
    switch (type) {
        case CONTENT_TEXT:
            return str_from_cstr("text/plain");
        case CONTENT_HTML:
            return str_from_cstr("text/html");
        case CONTENT_JSON:
            return str_from_cstr("application/json");
        case CONTENT_XML:
            return str_from_cstr("application/xml");
        case CONTENT_CSS:
            return str_from_cstr("application/css");
        case CONTENT_JAVASCRIPT:
            return str_from_cstr("application/javasript");
    }

    unreachable();
}

static str_t format_status_code(const http_status_code_t code) {
    switch (code) {
        case STATUS_OK:
            return str_from_cstr("200 OK");
        case STATUS_BAD_REQUEST:
            return str_from_cstr("400 Bad Request");
        case STATUS_FORBIDDEN:
            return str_from_cstr("403 Forbidden");
        case STATUS_NOT_FOUND:
            return str_from_cstr("404 Not Found");
        case STATUS_METHOD_NOT_ALLOWED:
            return str_from_cstr("405 Method Not Allowed");
        case STATUS_INTERNAL_SERVER_ERROR:
            return str_from_cstr("500 Internal Server Error");
        case STATUS_I_AM_A_TEAPOT:
            return str_from_cstr("418 I'm a teapot");
    }

    unreachable();
}

str_t load_html(const char* path) {
    str_t result = (str_t){0};
    FILE* f = fopen(path, "r");
    if (!f) {
        perror("Error loading html");
        return result;
    }

    fseek(f, 0, SEEK_END);
    result.len = ftell(f);
    fseek(f, 0, SEEK_SET);

    result.data = malloc(result.len);
    fread(result.data, sizeof(char), result.len, f);

    return result;
}

static void concat_header(dynamic_str_t* buf, const str_t key,
                          const str_t value) {
    string_concat(buf, str_trim(key));
    string_concat(buf, str_from_cstr(": "));
    string_concat(buf, str_trim(value));

    string_concat(buf, str_from_cstr("\r\n"));
}

static void concat_headers(dynamic_str_t* buf, const vector_t* headers) {
    for (usize i = 0; i < headers->size; ++i) {
        const str_pair_t* header = (str_pair_t*)headers->items + i;
        concat_header(buf, header->key, header->value);
    }
}

str_t const* ctx_get_request_header(const http_context_t* ctx,
                                    const str_t key) {
    return str_hashmap_get(&ctx->req.headers, key);
}

str_t const* ctx_get_parameter(const http_context_t* ctx, const str_t key) {
    return str_hashmap_get(&ctx->req.params, key);
}

void ctx_set_response_header(http_context_t* ctx, const str_t key,
                             const str_t value) {
    const str_pair_t header = {.key = str_trim(key), .value = str_trim(value)};
    vector_push(&ctx->writer.headers, sizeof(header), &header, 1);
}

void ctx_respond_empty(http_context_t* ctx, const http_status_code_t status) {
    dynamic_str_t buf = (dynamic_str_t){0};

    string_concat(&buf, str_from_cstr("HTTP/1.1 "));
    string_concat(&buf, format_status_code(status));
    string_concat(&buf, str_from_cstr("\r\n"));

    ctx_set_response_header(ctx, str_from_cstr("Connection"),
                            str_from_cstr("Close"));

    // push all of the headers into buf
    concat_headers(&buf, &ctx->writer.headers);

    string_concat(&buf, str_from_cstr("\r\n"));

    send(ctx->writer.conn_fd, buf.data, buf.size, 0);
    string_free(&buf);
}

void ctx_respond_msg(http_context_t* ctx, const http_status_code_t status,
                     const http_content_type_t content_type, const str_t body) {
    dynamic_str_t buf = (dynamic_str_t){0};
    char length_buf[16];

    string_concat(&buf, str_from_cstr("HTTP/1.1 "));
    string_concat(&buf, format_status_code(status));
    string_concat(&buf, str_from_cstr("\r\n"));

    ctx_set_response_header(ctx, str_from_cstr("Connection"),
                            str_from_cstr("Close"));

    sprintf(length_buf, "%lu", body.len);
    ctx_set_response_header(ctx, str_from_cstr("Content-Length"),
                            str_from_cstr(length_buf));
    ctx_set_response_header(ctx, str_from_cstr("Content-Type"),
                            format_content_type(content_type));

    // push all of the headers into buf
    concat_headers(&buf, &ctx->writer.headers);

    string_concat(&buf, str_from_cstr("\r\n"));
    string_concat(&buf, body);

    send(ctx->writer.conn_fd, buf.data, buf.size, 0);
    string_free(&buf);
}
