#include "context.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

dynamic_str_t load_html(const char* path) {
    dynamic_str_t result = (dynamic_str_t){0};
    FILE* f = fopen(path, "r");
    if (!f) {
        perror("Error loading html");
        return result;
    }

    fseek(f, 0, SEEK_END);
    const usize len = ftell(f);
    string_reserve(&result, len);
    fseek(f, 0, SEEK_SET);

    fread(result.data, sizeof(char), len, f);
    result.size = ftell(f);

    return result;
}

static void concat_header(dynamic_str_t* buf, const str_t key,
                          const str_t value) {
    string_concat(buf, str_trim(key));
    string_concat(buf, str_from_cstr(": "));
    string_concat(buf, str_trim(value));

    string_concat(buf, str_from_cstr("\r\n"));
}

static void concat_headers(dynamic_str_t* buf, const str_hashmap_t* headers) {
    for (usize i = 0; i < headers->bucket_count; ++i) {
        if (headers->metadata[i]) {
            const str_pair_t* header = headers->data + i;
            concat_header(buf, header->key, header->value);
        }
    }
}

str_t const* ctx_get_request_header(const http_context_t* ctx,
                                    const str_t key) {
    str_t lowered_key = (str_t){.data = malloc(key.len), .len = key.len};
    memcpy(lowered_key.data, key.data, key.len);
    str_to_lower(lowered_key);
    const str_t* result = str_hashmap_get(&ctx->req.headers, lowered_key);
    free(lowered_key.data);
    return result;
}

str_t const* ctx_get_parameter(const http_context_t* ctx, const str_t key) {
    str_t lowered_key = (str_t){.data = malloc(key.len), .len = key.len};
    memcpy(lowered_key.data, key.data, key.len);
    str_to_lower(lowered_key);
    const str_t* result = str_hashmap_get(&ctx->req.params, lowered_key);
    free(lowered_key.data);
    return result;
}

void ctx_set_response_header(http_context_t* ctx, const str_t key,
                             const str_t value) {
    const str_pair_t header = {.key = str_trim(key), .value = str_trim(value)};
    str_hashmap_insert_or_replace(&ctx->writer.headers, header.key,
                                  header.value);
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

    // empty line is required even if the body is empty
    string_concat(&buf, str_from_cstr("\r\n"));

    send(ctx->writer.conn_fd, buf.data, buf.size, 0);
    string_free(&buf);
}

void ctx_respond_msg(http_context_t* ctx, const http_status_code_t status,
                     const http_content_type_t content_type, const str_t body) {
    dynamic_str_t buf = (dynamic_str_t){0};

    string_concat(&buf, str_from_cstr("HTTP/1.1 "));
    string_concat(&buf, format_status_code(status));
    string_concat(&buf, str_from_cstr("\r\n"));

    ctx_set_response_header(ctx, str_from_cstr("Connection"),
                            str_from_cstr("Close"));
    ctx_set_response_header(ctx, str_from_cstr("Content-Type"),
                            format_content_type(content_type));
    char length_buf[32];
    str_t length = {.data = length_buf};
    length.len = snprintf(length_buf, sizeof(length_buf) / sizeof(char), "%lu",
                          body.len);
    ctx_set_response_header(ctx, str_from_cstr("Content-Length"), length);

    // push all of the headers into buf
    concat_headers(&buf, &ctx->writer.headers);

    // separate headers from body
    string_concat(&buf, str_from_cstr("\r\n"));
    string_concat(&buf, body);

    send(ctx->writer.conn_fd, buf.data, buf.size, 0);
    string_free(&buf);
}
