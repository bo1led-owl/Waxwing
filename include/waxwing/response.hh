#pragma once

#include <memory>
#include <optional>
#include <string>
#include <string_view>

#include "types.hh"

namespace waxwing {
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
    Response(const StatusCode code, internal::Headers&& headers,
             std::optional<Body>&& body) noexcept
        : status_code_{code},
          headers_{std::move(headers)},
          body_{std::move(body)} {}

    Response(const Response&) = delete;
    Response& operator=(const Response&) = delete;

    Response(Response&& rhs) {
        std::swap(status_code_, rhs.status_code_);
        std::swap(headers_, rhs.headers_);
        std::swap(body_, rhs.body_);
    }

    StatusCode status() const noexcept;
    internal::Headers& headers() noexcept;
    std::optional<Body>& body() noexcept;
};

class ResponseBuilder final {
    StatusCode status_code_;
    internal::Headers headers_;
    std::optional<Response::Body> body_;

public:
    ResponseBuilder(StatusCode code) noexcept : status_code_{code} {}

    ResponseBuilder(const ResponseBuilder&) = delete;
    ResponseBuilder& operator=(const ResponseBuilder&) = delete;

    template <typename S1, typename S2>
        requires(std::is_constructible_v<std::string, S1>) &&
                (std::is_constructible_v<std::string, S2>)
    ResponseBuilder& header(S1&& key, S2&& value) && noexcept {
        headers_.insert_or_assign(std::forward<S1>(key),
                                  std::forward<S2>(value));
        return *this;
    }

    template <typename S>
        requires(std::is_constructible_v<std::string, S>)
    ResponseBuilder& body(ContentType type, S&& data) && noexcept {
        body_ = Response::Body{type, std::forward<S>(data)};
        return *this;
    }

    template <typename S1, typename S2>
        requires(std::is_constructible_v<std::string, S1>) &&
                (std::is_constructible_v<std::string, S2>)
    ResponseBuilder& body(S1&& type, S2&& data) && noexcept {
        body_ = Response::Body{std::forward<S1>(type), std::forward<S2>(data)};
        return *this;
    }

    std::unique_ptr<Response> build() {
        return std::make_unique<Response>(status_code_, std::move(headers_),
                                          std::move(body_));
    }
};
}  // namespace waxwing
