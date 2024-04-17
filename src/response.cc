#include "response.hh"

#include <sys/socket.h>

#include <utility>

namespace http {

std::string_view format_content_type(const ContentType content_type) {
    switch (content_type) {
        case ContentType::Text:
            return "text/plain";
        case ContentType::Html:
            return "text/html";
        case ContentType::Json:
            return "application/json";
        case ContentType::Xml:
            return "application/xml";
        case ContentType::Css:
            return "application/css";
        case ContentType::Javascript:
            return "application/javasript";
    }

    std::unreachable();
}

std::string_view format_status(const StatusCode code) {
    switch (code) {
        case StatusCode::Continue:
            return "100 Continue";
        case StatusCode::SwitchingProtocols:
            return "101 Switching Protocols";
        case StatusCode::Processing:
            return "102 Processing";
        case StatusCode::EarlyHints:
            return "103 EarlyHints";
        case StatusCode::Ok:
            return "200 OK";
        case StatusCode::Created:
            return "201 Created";
        case StatusCode::Accepted:
            return "202 Accepted";
        case StatusCode::NonAuthoritativeInformation:
            return "203 Non-Authoritative Information";
        case StatusCode::NoContent:
            return "204 No Content";
        case StatusCode::ResetContent:
            return "205 Reset Content";
        case StatusCode::PartialContent:
            return "206 Partial Content";
        case StatusCode::MultiStatus:
            return "207 Multi-Status";
        case StatusCode::AlreadyReported:
            return "208 Already Reported";
        case StatusCode::IMUsed:
            return "226 IM Used";
        case StatusCode::MultipleChoices:
            return "300 Multiple Choices";
        case StatusCode::MovedPermanently:
            return "301 Moved Permanently";
        case StatusCode::Found:
            return "302 Found";
        case StatusCode::SeeOther:
            return "303 See Other";
        case StatusCode::NotModified:
            return "304 Not Modified";
        case StatusCode::TemporaryRedirect:
            return "307 Temporary Redirect";
        case StatusCode::PermanentRedirect:
            return "308 Permanent Redirect";
        case StatusCode::BadRequest:
            return "400 Bad Request";
        case StatusCode::Unauthorized:
            return "401 Unauthorized";
        case StatusCode::PaymentRequired:
            return "402 Payment Required";
        case StatusCode::Forbidden:
            return "403 Forbidden";
        case StatusCode::NotFound:
            return "404 Not Found";
        case StatusCode::MethodNotAllowed:
            return "405 Method Not Allowed";
        case StatusCode::NotAcceptable:
            return "406 Not Acceptable";
        case StatusCode::ProxyAuthenticationRequired:
            return "407 Proxy Authentication Required";
        case StatusCode::RequestTimeout:
            return "408 Request Timeout";
        case StatusCode::Conflict:
            return "409 Conflict";
        case StatusCode::Gone:
            return "410 Gone";
        case StatusCode::LengthRequired:
            return "411 Length Required";
        case StatusCode::PreconditionFailed:
            return "412 Precondition Failed";
        case StatusCode::ContentTooLarge:
            return "413 Content Too Large";
        case StatusCode::URITooLong:
            return "414 URI Too Long";
        case StatusCode::UnsupportedMediaType:
            return "415 Unsupported Media Type";
        case StatusCode::RangeNotSatisfiable:
            return "416 Range Not Satisfiable";
        case StatusCode::ExpectationFailed:
            return "417 Expectation Failed";
        case StatusCode::ImATeapot:
            return "418 I'a a teapot";
        case StatusCode::MisdirectedRequest:
            return "421 Misdirected Request";
        case StatusCode::UnprocessableContent:
            return "422 Unprocessable Content";
        case StatusCode::Locked:
            return "423 Locked";
        case StatusCode::FailedDependency:
            return "424 Failed Dependency";
        case StatusCode::TooEarly:
            return "425 Too Early";
        case StatusCode::UpgradeRequired:
            return "426 Upgrade Required";
        case StatusCode::PreconditionRequired:
            return "428 Precondition Required";
        case StatusCode::TooManyRequests:
            return "429 Too Many Requests";
        case StatusCode::RequestHeaderFieldsTooLarge:
            return "431 Request Header Fields Too Large";
        case StatusCode::UnavailableForLegalReasons:
            return "451 Unavailable For Legal Reasons";
        case StatusCode::InternalServerError:
            return "500 Internal Server Error";
        case StatusCode::NotImplemented:
            return "501 Not Implemented";
        case StatusCode::BadGateway:
            return "502 Bad Gateway";
        case StatusCode::ServiceUnavailable:
            return "503 Service Unavailable";
        case StatusCode::GatewayTimeout:
            return "504 Gateway Timeout";
        case StatusCode::HTTPVersionNotSupported:
            return "505 HTTP Version Not Supported";
        case StatusCode::VariantAlsoNegotiates:
            return "506 Variant Also Negotiates";
        case StatusCode::InsufficientStorage:
            return "507 Insufficient Storage";
        case StatusCode::LoopDetected:
            return "508 Loop Detected";
        case StatusCode::NotExtended:
            return "510 Not Extended";
        case StatusCode::NetworkAuthenticationRequired:
            return "511 Network Authentication Required";
    }
    std::unreachable();
}

Response& Response::header(const std::string& key, const std::string& value) {
    headers_.insert_or_assign(key, value);
    return *this;
}

Response& Response::body(const ContentType type, std::string&& data) {
    body_ = Response::Body{type, data};
    return *this;
}

Response& Response::body(const ContentType type, const std::string& data) {
    body_ = Response::Body{type, data};
    return *this;
}

Response& Response::body(const ContentType type, const std::string_view data) {
    body_ = Response::Body{type, std::string{data}};
    return *this;
}

Headers const& Response::get_headers() const& {
    return headers_;
}

StatusCode Response::get_status() const {
    return status_code_;
}

std::optional<Response::Body> const& Response::get_body() const& {
    return std::move(body_);
}
}  // namespace http
