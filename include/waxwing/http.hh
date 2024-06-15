#pragma once

#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>

#include "waxwing/str_util.hh"

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

        bool operator()(std::string_view lhs, std::string_view rhs) const {
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

    template <typename K, typename V>
        requires(std::constructible_from<std::string, K>) &&
                (std::constructible_from<std::string, V>)
    void insert_or_assign(K&& key, V&& value) {
        auto [it, success] =
            repr_.try_emplace(std::forward<K>(key), std::forward<V>(value));
        if (!success) {
            it->second = std::forward<V>(value);
        }
    }

    bool contains(std::string_view key) const noexcept;
    std::optional<std::string_view> get(std::string_view key) const noexcept;

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
    Continue_100 = 100,
    SwitchingProtocols_101 = 101,
    Processing_102 = 102,
    EarlyHints_103 = 103,
    Ok_200 = 200,
    Created_201 = 201,
    Accepted_202 = 202,
    NonAuthoritativeInformation_203 = 203,
    NoContent_204 = 204,
    ResetContent_205 = 205,
    PartialContent_206 = 206,
    MultiStatus_207 = 207,
    AlreadyReported_208 = 208,
    IMUsed_226 = 226,
    MultipleChoices_300 = 300,
    MovedPermanently_301 = 301,
    Found_302 = 302,
    SeeOther_303 = 303,
    NotModified_304 = 304,
    TemporaryRedirect_307 = 307,
    PermanentRedirect_308 = 308,
    BadRequest_400 = 400,
    Unauthorized_401 = 401,
    PaymentRequired_402 = 402,
    Forbidden_403 = 403,
    NotFound_404 = 404,
    MethodNotAllowed_405 = 405,
    NotAcceptable_406 = 406,
    ProxyAuthenticationRequired_407 = 407,
    RequestTimeout_408 = 408,
    Conflict_409 = 409,
    Gone_410 = 410,
    LengthRequired_411 = 411,
    PreconditionFailed_412 = 412,
    ContentTooLarge_413 = 413,
    URITooLong_414 = 414,
    UnsupportedMediaType_415 = 415,
    RangeNotSatisfiable_416 = 416,
    ExpectationFailed_417 = 417,
    ImATeapot_418 = 418,
    MisdirectedRequest_421 = 421,
    UnprocessableContent_422 = 422,
    Locked_423 = 423,
    FailedDependency_424 = 424,
    TooEarly_425 = 425,
    UpgradeRequired_426 = 426,
    PreconditionRequired_428 = 428,
    TooManyRequests_429 = 429,
    RequestHeaderFieldsTooLarge_431 = 431,
    UnavailableForLegalReasons_451 = 451,
    InternalServerError_500 = 500,
    NotImplemented_501 = 501,
    BadGateway_502 = 502,
    ServiceUnavailable_503 = 503,
    GatewayTimeout_504 = 504,
    HTTPVersionNotSupported_505 = 505,
    VariantAlsoNegotiates_506 = 506,
    InsufficientStorage_507 = 507,
    LoopDetected_508 = 508,
    NotExtended_510 = 510,
    NetworkAuthenticationRequired_511 = 511,
};

std::string_view format_method(HttpMethod method) noexcept;
std::optional<HttpMethod> parse_method(std::string_view s) noexcept;
std::string_view format_status_code(HttpStatusCode code) noexcept;

namespace content_type {
constexpr std::string_view plaintext = "text/plain";
constexpr std::string_view html = "text/html";
constexpr std::string_view javascript = "text/javascript";
constexpr std::string_view css = "text/css";
constexpr std::string_view json = "application/json";
constexpr std::string_view csv = "text/csv";
constexpr std::string_view mp3 = "audio/mpeg";
constexpr std::string_view mp4 = "video/mp4";
constexpr std::string_view ico = "image/vnd.microsoft.icon";
constexpr std::string_view jpeg = "image/jpeg";
constexpr std::string_view png = "image/png";
constexpr std::string_view gif = "image/gif";
}  // namespace content_type
}  // namespace waxwing
