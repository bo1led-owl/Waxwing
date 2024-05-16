#include "waxwing/http.hh"

#include <optional>
#include <string_view>

namespace waxwing {
bool Headers::contains(const std::string_view key) const noexcept {
    return repr_.contains(key);
}

std::optional<std::string_view> Headers::get(
    const std::string_view key) const noexcept {
    const auto iter = repr_.find(key);
    if (iter == repr_.end()) {
        return std::nullopt;
    }

    return iter->second;
}

std::string_view format_method(const HttpMethod method) noexcept {
    switch (method) {
        case HttpMethod::Get:
            return "GET";
        case HttpMethod::Post:
            return "POST";
        case HttpMethod::Head:
            return "HEAD";
        case HttpMethod::Put:
            return "PUT";
        case HttpMethod::Delete:
            return "DELETE";
        case HttpMethod::Connect:
            return "CONNECT";
        case HttpMethod::Options:
            return "OPTIONS";
        case HttpMethod::Trace:
            return "TRACE";
        case HttpMethod::Patch:
            return "PATCH";
    }
    __builtin_unreachable();
}

std::optional<HttpMethod> parse_method(const std::string_view s) noexcept {
    if (s == "GET") {
        return HttpMethod::Get;
    }
    if (s == "POST") {
        return HttpMethod::Post;
    }
    if (s == "HEAD") {
        return HttpMethod::Head;
    }
    if (s == "PUT") {
        return HttpMethod::Put;
    }
    if (s == "DELETE") {
        return HttpMethod::Delete;
    }
    if (s == "CONNECT") {
        return HttpMethod::Connect;
    }
    if (s == "OPTIONS") {
        return HttpMethod::Options;
    }
    if (s == "TRACE") {
        return HttpMethod::Trace;
    }
    if (s == "PATCH") {
        return HttpMethod::Patch;
    }
    return std::nullopt;
}

std::string_view format_status_code(const HttpStatusCode code) noexcept {
    switch (code) {
        case HttpStatusCode::Continue:
            return "100 Continue";
        case HttpStatusCode::SwitchingProtocols:
            return "101 Switching Protocols";
        case HttpStatusCode::Processing:
            return "102 Processing";
        case HttpStatusCode::EarlyHints:
            return "103 EarlyHints";
        case HttpStatusCode::Ok:
            return "200 OK";
        case HttpStatusCode::Created:
            return "201 Created";
        case HttpStatusCode::Accepted:
            return "202 Accepted";
        case HttpStatusCode::NonAuthoritativeInformation:
            return "203 Non-Authoritative Information";
        case HttpStatusCode::NoContent:
            return "204 No Content";
        case HttpStatusCode::ResetContent:
            return "205 Reset Content";
        case HttpStatusCode::PartialContent:
            return "206 Partial Content";
        case HttpStatusCode::MultiStatus:
            return "207 Multi-Status";
        case HttpStatusCode::AlreadyReported:
            return "208 Already Reported";
        case HttpStatusCode::IMUsed:
            return "226 IM Used";
        case HttpStatusCode::MultipleChoices:
            return "300 Multiple Choices";
        case HttpStatusCode::MovedPermanently:
            return "301 Moved Permanently";
        case HttpStatusCode::Found:
            return "302 Found";
        case HttpStatusCode::SeeOther:
            return "303 See Other";
        case HttpStatusCode::NotModified:
            return "304 Not Modified";
        case HttpStatusCode::TemporaryRedirect:
            return "307 Temporary Redirect";
        case HttpStatusCode::PermanentRedirect:
            return "308 Permanent Redirect";
        case HttpStatusCode::BadRequest:
            return "400 Bad Request";
        case HttpStatusCode::Unauthorized:
            return "401 Unauthorized";
        case HttpStatusCode::PaymentRequired:
            return "402 Payment Required";
        case HttpStatusCode::Forbidden:
            return "403 Forbidden";
        case HttpStatusCode::NotFound:
            return "404 Not Found";
        case HttpStatusCode::MethodNotAllowed:
            return "405 Method Not Allowed";
        case HttpStatusCode::NotAcceptable:
            return "406 Not Acceptable";
        case HttpStatusCode::ProxyAuthenticationRequired:
            return "407 Proxy Authentication Required";
        case HttpStatusCode::RequestTimeout:
            return "408 Request Timeout";
        case HttpStatusCode::Conflict:
            return "409 Conflict";
        case HttpStatusCode::Gone:
            return "410 Gone";
        case HttpStatusCode::LengthRequired:
            return "411 Length Required";
        case HttpStatusCode::PreconditionFailed:
            return "412 Precondition Failed";
        case HttpStatusCode::ContentTooLarge:
            return "413 Content Too Large";
        case HttpStatusCode::URITooLong:
            return "414 URI Too Long";
        case HttpStatusCode::UnsupportedMediaType:
            return "415 Unsupported Media Type";
        case HttpStatusCode::RangeNotSatisfiable:
            return "416 Range Not Satisfiable";
        case HttpStatusCode::ExpectationFailed:
            return "417 Expectation Failed";
        case HttpStatusCode::ImATeapot:
            return "418 I'a a teapot";
        case HttpStatusCode::MisdirectedRequest:
            return "421 Misdirected Request";
        case HttpStatusCode::UnprocessableContent:
            return "422 Unprocessable Content";
        case HttpStatusCode::Locked:
            return "423 Locked";
        case HttpStatusCode::FailedDependency:
            return "424 Failed Dependency";
        case HttpStatusCode::TooEarly:
            return "425 Too Early";
        case HttpStatusCode::UpgradeRequired:
            return "426 Upgrade Required";
        case HttpStatusCode::PreconditionRequired:
            return "428 Precondition Required";
        case HttpStatusCode::TooManyRequests:
            return "429 Too Many Requests";
        case HttpStatusCode::RequestHeaderFieldsTooLarge:
            return "431 Request Header Fields Too Large";
        case HttpStatusCode::UnavailableForLegalReasons:
            return "451 Unavailable For Legal Reasons";
        case HttpStatusCode::InternalServerError:
            return "500 Internal Server Error";
        case HttpStatusCode::NotImplemented:
            return "501 Not Implemented";
        case HttpStatusCode::BadGateway:
            return "502 Bad Gateway";
        case HttpStatusCode::ServiceUnavailable:
            return "503 Service Unavailable";
        case HttpStatusCode::GatewayTimeout:
            return "504 Gateway Timeout";
        case HttpStatusCode::HTTPVersionNotSupported:
            return "505 HTTP Version Not Supported";
        case HttpStatusCode::VariantAlsoNegotiates:
            return "506 Variant Also Negotiates";
        case HttpStatusCode::InsufficientStorage:
            return "507 Insufficient Storage";
        case HttpStatusCode::LoopDetected:
            return "508 Loop Detected";
        case HttpStatusCode::NotExtended:
            return "510 Not Extended";
        case HttpStatusCode::NetworkAuthenticationRequired:
            return "511 Network Authentication Required";
    }
    __builtin_unreachable();
}

namespace content_type {
std::string_view plaintext() noexcept { return "text/plain"; }
std::string_view html() noexcept { return "text/html"; }
std::string_view javascript() noexcept { return "text/javascript"; }
std::string_view css() noexcept { return "text/css"; }
std::string_view json() noexcept { return "application/json"; }
std::string_view csv() noexcept { return "text/csv"; }
std::string_view mp3() noexcept { return "audio/mpeg"; }
std::string_view mp4() noexcept { return "video/mp4"; }
std::string_view ico() noexcept { return "image/vnd.microsoft.icon"; }
std::string_view jpeg() noexcept { return "image/jpeg"; }
std::string_view png() noexcept { return "image/png"; }
std::string_view gif() noexcept { return "image/gif"; }
}  // namespace content_type
}  // namespace waxwing
