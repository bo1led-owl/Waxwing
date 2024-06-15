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
        case HttpStatusCode::Continue_100:
            return "100 Continue";
        case HttpStatusCode::SwitchingProtocols_101:
            return "101 Switching Protocols";
        case HttpStatusCode::Processing_102:
            return "102 Processing";
        case HttpStatusCode::EarlyHints_103:
            return "103 EarlyHints";
        case HttpStatusCode::Ok_200:
            return "200 OK";
        case HttpStatusCode::Created_201:
            return "201 Created";
        case HttpStatusCode::Accepted_202:
            return "202 Accepted";
        case HttpStatusCode::NonAuthoritativeInformation_203:
            return "203 Non-Authoritative Information";
        case HttpStatusCode::NoContent_204:
            return "204 No Content";
        case HttpStatusCode::ResetContent_205:
            return "205 Reset Content";
        case HttpStatusCode::PartialContent_206:
            return "206 Partial Content";
        case HttpStatusCode::MultiStatus_207:
            return "207 Multi-Status";
        case HttpStatusCode::AlreadyReported_208:
            return "208 Already Reported";
        case HttpStatusCode::IMUsed_226:
            return "226 IM Used";
        case HttpStatusCode::MultipleChoices_300:
            return "300 Multiple Choices";
        case HttpStatusCode::MovedPermanently_301:
            return "301 Moved Permanently";
        case HttpStatusCode::Found_302:
            return "302 Found";
        case HttpStatusCode::SeeOther_303:
            return "303 See Other";
        case HttpStatusCode::NotModified_304:
            return "304 Not Modified";
        case HttpStatusCode::TemporaryRedirect_307:
            return "307 Temporary Redirect";
        case HttpStatusCode::PermanentRedirect_308:
            return "308 Permanent Redirect";
        case HttpStatusCode::BadRequest_400:
            return "400 Bad Request";
        case HttpStatusCode::Unauthorized_401:
            return "401 Unauthorized";
        case HttpStatusCode::PaymentRequired_402:
            return "402 Payment Required";
        case HttpStatusCode::Forbidden_403:
            return "403 Forbidden";
        case HttpStatusCode::NotFound_404:
            return "404 Not Found";
        case HttpStatusCode::MethodNotAllowed_405:
            return "405 Method Not Allowed";
        case HttpStatusCode::NotAcceptable_406:
            return "406 Not Acceptable";
        case HttpStatusCode::ProxyAuthenticationRequired_407:
            return "407 Proxy Authentication Required";
        case HttpStatusCode::RequestTimeout_408:
            return "408 Request Timeout";
        case HttpStatusCode::Conflict_409:
            return "409 Conflict";
        case HttpStatusCode::Gone_410:
            return "410 Gone";
        case HttpStatusCode::LengthRequired_411:
            return "411 Length Required";
        case HttpStatusCode::PreconditionFailed_412:
            return "412 Precondition Failed";
        case HttpStatusCode::ContentTooLarge_413:
            return "413 Content Too Large";
        case HttpStatusCode::URITooLong_414:
            return "414 URI Too Long";
        case HttpStatusCode::UnsupportedMediaType_415:
            return "415 Unsupported Media Type";
        case HttpStatusCode::RangeNotSatisfiable_416:
            return "416 Range Not Satisfiable";
        case HttpStatusCode::ExpectationFailed_417:
            return "417 Expectation Failed";
        case HttpStatusCode::ImATeapot_418:
            return "418 I'a a teapot";
        case HttpStatusCode::MisdirectedRequest_421:
            return "421 Misdirected Request";
        case HttpStatusCode::UnprocessableContent_422:
            return "422 Unprocessable Content";
        case HttpStatusCode::Locked_423:
            return "423 Locked";
        case HttpStatusCode::FailedDependency_424:
            return "424 Failed Dependency";
        case HttpStatusCode::TooEarly_425:
            return "425 Too Early";
        case HttpStatusCode::UpgradeRequired_426:
            return "426 Upgrade Required";
        case HttpStatusCode::PreconditionRequired_428:
            return "428 Precondition Required";
        case HttpStatusCode::TooManyRequests_429:
            return "429 Too Many Requests";
        case HttpStatusCode::RequestHeaderFieldsTooLarge_431:
            return "431 Request Header Fields Too Large";
        case HttpStatusCode::UnavailableForLegalReasons_451:
            return "451 Unavailable For Legal Reasons";
        case HttpStatusCode::InternalServerError_500:
            return "500 Internal Server Error";
        case HttpStatusCode::NotImplemented_501:
            return "501 Not Implemented";
        case HttpStatusCode::BadGateway_502:
            return "502 Bad Gateway";
        case HttpStatusCode::ServiceUnavailable_503:
            return "503 Service Unavailable";
        case HttpStatusCode::GatewayTimeout_504:
            return "504 Gateway Timeout";
        case HttpStatusCode::HTTPVersionNotSupported_505:
            return "505 HTTP Version Not Supported";
        case HttpStatusCode::VariantAlsoNegotiates_506:
            return "506 Variant Also Negotiates";
        case HttpStatusCode::InsufficientStorage_507:
            return "507 Insufficient Storage";
        case HttpStatusCode::LoopDetected_508:
            return "508 Loop Detected";
        case HttpStatusCode::NotExtended_510:
            return "510 Not Extended";
        case HttpStatusCode::NetworkAuthenticationRequired_511:
            return "511 Network Authentication Required";
    }
    __builtin_unreachable();
}
}  // namespace waxwing
