#pragma once

#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>

#include "str_util.hh"

namespace waxwing {
class Headers {
    struct string_hash {
        using hash_type = std::hash<std::string_view>;
        using is_transparent = std::true_type;

        size_t operator()(const char* str) const { return hash_type{}(str); }
        size_t operator()(std::string_view str) const {
            return hash_type{}(str);
        }
        size_t operator()(std::string const& str) const {
            return hash_type{}(str);
        }
    };
    struct eq_comparator {
        using is_transparent = std::true_type;

        constexpr bool operator()(std::string_view lhs,
                                  std::string_view rhs) const {
            return str_util::case_insensitive_eq(lhs, rhs);
        }
    };
    using ReprType = std::unordered_map<std::string, std::string, string_hash,
                                        eq_comparator>;
    ReprType repr_;

public:
    Headers() = default;

    template <typename K, typename V>
        requires(std::constructible_from<std::string, K>) &&
                (std::constructible_from<std::string, V>)
    void insert(K&& key, V&& value) {
        repr_.try_emplace(std::forward<K>(key), std::forward<V>(value));
    }

    bool contains(std::string_view key) const noexcept;
    std::optional<std::string_view> get(std::string_view key) const noexcept;

    template <typename S>
        requires(std::constructible_from<std::string, S>)
    std::string& operator[](S key) {
        auto iter = repr_.find(std::forward<S>(key));
        if (iter != repr_.end()) {
            return iter->second;
        } else {
            return repr_.emplace(std::string{std::forward<S>(key)}, "")
                .first->second;
        }
    }

    ReprType::iterator begin() noexcept { return repr_.begin(); }
    ReprType::iterator end() noexcept { return repr_.end(); }
    ReprType::const_iterator begin() const noexcept { return repr_.cbegin(); }
    ReprType::const_iterator end() const noexcept { return repr_.cend(); }
    ReprType::const_iterator cbegin() const noexcept { return repr_.cbegin(); }
    ReprType::const_iterator cend() const noexcept { return repr_.cend(); }
};

using PathParameters = std::span<std::string_view const>;

enum class HttpMethod {
    Get,
    Head,
    Post,
    Put,
    Delete,
    Connect,
    Options,
    Trace,
    Patch,
};

enum class HttpStatusCode {
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

std::string_view format_method(HttpMethod method) noexcept;
std::optional<HttpMethod> parse_method(std::string_view s) noexcept;
std::string_view format_status_code(HttpStatusCode code) noexcept;

namespace content_type {
std::string_view plaintext() noexcept;
std::string_view html() noexcept;
std::string_view javascript() noexcept;
std::string_view css() noexcept;
std::string_view json() noexcept;
std::string_view csv() noexcept;
std::string_view mp3() noexcept;
std::string_view mp4() noexcept;
std::string_view ico() noexcept;
std::string_view jpeg() noexcept;
std::string_view png() noexcept;
std::string_view gif() noexcept;
}  // namespace content_type
}  // namespace waxwing
