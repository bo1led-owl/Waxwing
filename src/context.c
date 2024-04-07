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
        default:
            unreachable();
    }
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
        default:
            unreachable();
    }
}

void print_method(http_method_t method) {
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

void print_req(const http_request_t* req) {
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

http_header_t parse_header(const str_t input) {
    usize colon_pos = 0;
    for (; colon_pos < input.len; ++colon_pos) {
        if (input.data[colon_pos] == ':')
            break;
    }

    return (http_header_t){
        .key = str_trim((str_t){.data = input.data, .len = colon_pos}),
        .value = str_trim((str_t){.data = input.data + colon_pos + 1,
                                  .len = input.len - colon_pos - 1})};
}

bool parse_request(const str_t req, http_request_t* result) {
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
        const http_header_t header = parse_header(line);
        headers_set(&result->headers, header.key, header.value);
    }

    result->body = str_trim(line_iter.source);
    return true;
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

static void concat_header(dynamic_str_t* buf, const http_header_t* header) {
    string_concat(buf, str_trim(header->key));
    string_concat(buf, str_from_cstr(": "));
    string_concat(buf, str_trim(header->value));

    string_concat(buf, str_from_cstr("\r\n"));
}

static void concat_headers(dynamic_str_t* buf, const vector_t* headers) {
    for (usize i = 0; i < headers->size; ++i) {
        concat_header(buf, (http_header_t*)headers->items + i);
    }
}

str_t* ctx_get_request_header(http_context_t* ctx, const str_t key) {
    return headers_get(&ctx->req.headers, key);
}

void ctx_set_response_header(http_context_t* ctx, const str_t key,
                             const str_t value) {
    http_header_t header = {.key = str_trim(key), .value = str_trim(value)};
    vector_push(&ctx->writer.headers, sizeof(header), &header, 1);
}

void ctx_respond_status(http_context_t* ctx, const http_status_code_t status) {
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

void ctx_free(http_context_t* ctx) {
    headers_free(&ctx->req.headers);
    *ctx = (http_context_t){0};
}
