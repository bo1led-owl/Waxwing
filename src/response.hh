#pragma once

#include <optional>
#include <string>
#include <string_view>

#include "types.hh"

namespace http {
enum class ContentType {
    Text,
    Html,
    Json,
    Xml,
    Css,
    Javascript,
};

enum class StatusCode {
    Continue = 100,
    SwitchingProtocols = 101,
    Processing = 102,
    EarlyHints = 103,
    Ok = 200,
    Created = 201,
    Accepted = 202,
    NonAuthoritativeInformation = 203,
    NoContent = 204,
    ResetContent = 205,
    PartialContent = 206,
    MultiStatus = 207,
    AlreadyReported = 208,
    IMUsed = 226,
    MultipleChoices = 300,
    MovedPermanently = 301,
    Found = 302,
    SeeOther = 303,
    NotModified = 304,
    TemporaryRedirect = 307,
    PermanentRedirect = 308,
    BadRequest = 400,
    Unauthorized = 401,
    PaymentRequired = 402,
    Forbidden = 403,
    NotFound = 404,
    MethodNotAllowed = 405,
    NotAcceptable = 406,
    ProxyAuthenticationRequired = 407,
    RequestTimeout = 408,
    Conflict = 409,
    Gone = 410,
    LengthRequired = 411,
    PreconditionFailed = 412,
    ContentTooLarge = 413,
    URITooLong = 414,
    UnsupportedMediaType = 415,
    RangeNotSatisfiable = 416,
    ExpectationFailed = 417,
    ImATeapot = 418,
    MisdirectedRequest = 421,
    UnprocessableContent = 422,
    Locked = 423,
    FailedDependency = 424,
    TooEarly = 425,
    UpgradeRequired = 426,
    PreconditionRequired = 428,
    TooManyRequests = 429,
    RequestHeaderFieldsTooLarge = 431,
    UnavailableForLegalReasons = 451,
    InternalServerError = 500,
    NotImplemented = 501,
    BadGateway = 502,
    ServiceUnavailable = 503,
    GatewayTimeout = 504,
    HTTPVersionNotSupported = 505,
    VariantAlsoNegotiates = 506,
    InsufficientStorage = 507,
    LoopDetected = 508,
    NotExtended = 510,
    NetworkAuthenticationRequired = 511,
};

std::string_view format_content_type(ContentType content_type);
std::string_view format_status(StatusCode status);

class Response {
public:
    struct Body {
        ContentType type;
        std::string data;

        Body(const ContentType type, const std::string& data)
            : type{type}, data{data} {}
        Body(const ContentType type, std::string&& data)
            : type{type}, data{data} {}
    };

private:
    StatusCode status_code_;
    Headers headers_;
    std::optional<Body> body_;

public:
    Response(const StatusCode code) : status_code_{code} {}

    Response& header(const std::string& key, const std::string& value);
    Response& body(const ContentType type, std::string&& data);
    Response& body(const ContentType type, const std::string& data);
    Response& body(const ContentType type, const std::string_view data);
    StatusCode get_status() const;
    Headers const& get_headers() const&;
    std::optional<Body> const& get_body() const&;
};
}  // namespace http
