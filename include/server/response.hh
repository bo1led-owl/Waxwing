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

std::string_view format_content_type(ContentType content_type) noexcept;
std::string_view format_status(StatusCode code) noexcept;

class Response {
public:
    struct Body {
        std::string type;
        std::string data;

        template <typename T>
            requires(std::is_constructible_v<std::string, T>)
        Body(const ContentType type, T&& data)
            : type{format_content_type(type)}, data{std::forward<T>(data)} {}

        template <typename T, typename U>
            requires(std::is_constructible_v<std::string, T>) &&
                        (std::is_constructible_v<std::string, U>)
        Body(T&& type, U&& data) : type{type}, data{std::forward<T>(data)} {}
    };

private:
    StatusCode status_code_;
    internal::Headers headers_;
    std::optional<Body> body_;

public:
    Response(const StatusCode code) noexcept : status_code_{code} {}

    template <typename T, typename U>
        requires(std::is_constructible_v<std::string, T>) &&
                (std::is_constructible_v<std::string, U>)
    Response& header(T&& key, U&& value) noexcept {
        headers_.insert_or_assign(std::forward<T>(key), std::forward<U>(value));
        return *this;
    }

    template <typename T>
        requires(std::is_constructible_v<std::string, T>)
    Response& body(ContentType type, T&& data) noexcept {
        body_ = Body{type, std::forward<T>(data)};
        return *this;
    }

    template <typename T, typename U>
        requires(std::is_constructible_v<std::string, T>) &&
                (std::is_constructible_v<std::string, U>)
    Response& body(T&& type, U&& data) noexcept {
        body_ = Body{std::forward<T>(type), std::forward<U>(data)};
        return *this;
    }

    StatusCode get_status() const noexcept;
    internal::Headers const& get_headers() const& noexcept;
    std::optional<Body> const& get_body() const& noexcept;
};
}  // namespace http
