#pragma once

#include <functional>
#include <optional>
#include <stdexcept>
#include <type_traits>
#include <variant>

namespace waxwing {
template <typename E>
class [[nodiscard]] Error;
template <typename T, typename E>
class [[nodiscard(
    "`Result` object is ignored, it might have been an error")]] Result;

namespace result {
template <typename T>
constexpr bool is_optional = false;
template <typename T>
constexpr bool is_optional<std::optional<T>> = true;

template <typename G>
constexpr bool is_error = false;
template <typename G>
constexpr bool is_error<Error<G>> = true;

template <typename U>
constexpr bool is_result = false;
template <typename U, typename G>
constexpr bool is_result<Result<U, G>> = true;

template <typename T>
constexpr bool can_be_error =
    std::is_object_v<T> && !std::is_array_v<T> && !std::is_const_v<T> &&
    !std::is_volatile_v<T> && !std::is_reference_v<T>;

template <typename T>
constexpr bool can_be_value =
    (std::is_object_v<T> && !std::is_array_v<T> && !std::is_const_v<T> &&
     !std::is_volatile_v<T> && !std::is_reference_v<T> && !is_error<T>) ||
    std::is_void_v<T>;
}  // namespace result

template <typename E>
class Error final {
    static_assert(!result::is_error<E> && result::can_be_error<E>);

    E value_;

public:
    template <typename Err>
        requires(!result::is_error<Err>) && (std::constructible_from<E, Err>)
    explicit constexpr Error(Err&& err) : value_{std::forward<Err>(err)} {}

    [[nodiscard]] constexpr E& error() & noexcept { return value_; }
    [[nodiscard]] constexpr E const& error() const& noexcept { return value_; }
    [[nodiscard]] constexpr E&& error() && noexcept {
        return std::move(value_);
    }
    [[nodiscard]] constexpr E const&& error() const&& noexcept {
        return std::move(value_);
    }
};

template <typename E>
Error(E) -> Error<E>;

template <typename T, typename E>
class Result final {
    template <typename, typename>
    friend class Result;

    static_assert(!result::is_result<T> && !result::is_result<E> &&
                  !result::is_error<T> && !result::is_error<E> &&
                  result::can_be_value<T> && result::can_be_error<E>);

    static constexpr size_t VALUE = 0;
    static constexpr size_t ERROR = 1;

    using value_type = std::conditional_t<std::is_void_v<T>, std::monostate, T>;
    using error_type = E;
    std::variant<value_type, error_type> value_;

public:
    constexpr Result()
        requires(std::is_void_v<T> || std::default_initializable<T>)
        : value_{} {}

    constexpr Result(const Result&) = default;
    constexpr Result(Result&&) = default;

    constexpr Result& operator=(const Result&) = default;
    constexpr Result& operator=(Result&&) = default;

    template <typename U = T>
        requires(!std::same_as<std::remove_cvref_t<U>, Result>) &&
                (!result::is_result<U>) && (!result::is_error<U>) &&
                (std::constructible_from<T, U>)
    constexpr explicit(!std::convertible_to<U, T>) Result(U&& value)
        : value_{std::in_place_index<VALUE>, std::forward<U>(value)} {}

    template <typename G = E>
        requires(std::constructible_from<E, G>) && (!result::is_error<G>)
    constexpr explicit(!std::convertible_to<G, E>) Result(const Error<G>& error)
        : value_{std::in_place_index<ERROR>, error.error()} {}

    template <typename G = E>
        requires(std::constructible_from<E, G>)
    constexpr explicit(!std::convertible_to<G, E>) Result(Error<G>&& error)
        : value_{std::in_place_index<ERROR>, std::move(error).error()} {}

    template <typename U = T, typename G = E>
        requires(std::constructible_from<T, const U&>) &&
                (std::constructible_from<E, const G&>)
    constexpr explicit(!std::convertible_to<U, T> || !std::convertible_to<G, E>)
        Result(const Result<U, G>& res) {
        if (res.has_value()) {
            value_.template emplace<VALUE>(std::get<VALUE>(res.value_));
        } else {
            value_.template emplace<ERROR>(std::get<ERROR>(res.value_));
        }
    }

    template <typename U = T, typename G = E>
        requires(std::constructible_from<T, const U&>) &&
                (std::constructible_from<E, const G&>)
    constexpr explicit(!std::convertible_to<U, T> || !std::convertible_to<G, E>)
        Result(Result<U, G>&& res) {
        value_.swap(std::move(res).value_);
    }

    constexpr bool has_value() const { return value_.index() == VALUE; }
    constexpr bool has_error() const { return value_.index() == ERROR; }
    constexpr operator bool() const { return has_value(); }

    constexpr operator value_type() const = delete;
    constexpr operator error_type() const
        requires(!std::is_same_v<value_type, error_type>)
    = delete;

    [[nodiscard]] constexpr const T* operator->() const noexcept
        requires(!std::is_void_v<T>)
    {
        return std::addressof(std::get<VALUE>(value_));
    }
    [[nodiscard]] constexpr T* operator->() noexcept
        requires(!std::is_void_v<T>)
    {
        return std::addressof(std::get<VALUE>(value_));
    }

    [[nodiscard]] constexpr const value_type& operator*() const& noexcept {
        return std::get<VALUE>(value_);
    }
    [[nodiscard]] constexpr value_type& operator*() & noexcept {
        return std::get<VALUE>(value_);
    }
    [[nodiscard]] constexpr const value_type&& operator*() const&& noexcept {
        return std::move(std::get<VALUE>(std::move(value_)));
    }
    [[nodiscard]] constexpr value_type&& operator*() && noexcept {
        return std::move(std::get<VALUE>(std::move(value_)));
    }

    [[nodiscard]] constexpr value_type& value() &
        requires(!std::is_void_v<T>)
    {
        if (!has_value()) [[unlikely]] {
            throw std::logic_error(
                "Accessing `value` of `Result` which doesn't have a value");
        }
        return std::get<VALUE>(value_);
    }
    [[nodiscard]] constexpr value_type const& value() const&
        requires(!std::is_void_v<T>)
    {
        if (!has_value()) [[unlikely]] {
            throw std::logic_error(
                "Accessing `value` of `Result` which doesn't have a value");
        }
        return std::get<VALUE>(value_);
    }
    [[nodiscard]] constexpr value_type&& value() &&
        requires(!std::is_void_v<T>)
    {
        if (!has_value()) [[unlikely]] {
            throw std::logic_error(
                "Accessing `value` of `Result` which doesn't have a value");
        }
        return std::move(std::get<VALUE>(std::move(value_)));
    }
    [[nodiscard]] constexpr value_type const&& value() const&&
        requires(!std::is_void_v<T>)
    {
        if (!has_value()) [[unlikely]] {
            throw std::logic_error(
                "Accessing `value` of `Result` which doesn't have a value");
        }
        return std::move(std::get<VALUE>(std::move(value_)));
    }

    template <typename U = T>
    [[nodiscard]] constexpr value_type value_or(U&& default_value) const&
        requires(!std::is_void_v<T>) && (std::constructible_from<T, U>)
    {
        if (has_value()) {
            return std::get<VALUE>(value_);
        }
        return value_type{std::forward<U>(default_value)};
    }

    template <typename U = T>
    [[nodiscard]] constexpr value_type value_or(U&& default_value) &&
        requires(!std::is_void_v<T>) && (std::constructible_from<T, U>)
    {
        if (has_value()) {
            return std::move(std::get<VALUE>(std::move(value_)));
        }
        return value_type{std::forward<U>(default_value)};
    }

    [[nodiscard]] constexpr error_type& error() &
        requires(!std::is_void_v<E>)
    {
        return std::get<ERROR>(value_);
    }
    [[nodiscard]] constexpr error_type const& error() const&
        requires(!std::is_void_v<E>)
    {
        if (has_value()) [[unlikely]] {
            throw std::logic_error(
                "Accessing `error` of `Result` which has a value");
        }
        return std::get<ERROR>(value_);
    }
    [[nodiscard]] constexpr error_type&& error() &&
        requires(!std::is_void_v<E>)
    {
        if (has_value()) [[unlikely]] {
            throw std::logic_error(
                "Accessing `error` of `Result` which has a value");
        }
        return std::move(std::get<ERROR>(std::move(value_)));
    }
    [[nodiscard]] constexpr error_type const&& error() const&&
        requires(!std::is_void_v<E>)
    {
        if (has_value()) [[unlikely]] {
            throw std::logic_error(
                "Accessing `error` of `Result` which has a value");
        }
        return std::move(std::get<ERROR>(std::move(value_)));
    }

    template <std::invocable F>
    constexpr Result<std::invoke_result_t<F, value_type const&>, E> map(
        F&& f) const& {
        if (has_value()) {
            return Result{
                std::invoke(std::forward<F>(f), std::get<VALUE>(value_))};
        }
        return *this;
    }

    template <std::invocable F>
    constexpr Result<std::invoke_result_t<F, value_type&&>, E> map(F&& f) && {
        if (has_value()) {
            return Result{
                std::invoke(std::forward<F>(f),
                            std::move(std::get<VALUE>(std::move(value_))))};
        }
        return *this;
    }

    template <std::invocable F>
    constexpr Result<
        typename std::invoke_result_t<F, value_type const&>::value_type, E>
    and_then(F&& f) const& {
        if (has_value()) {
            return std::invoke(std::forward<F>(f), std::get<VALUE>(value_));
        }
        return *this;
    }

    template <std::invocable F>
    constexpr Result<typename std::invoke_result_t<F, value_type&&>::value_type,
                     E>
    and_then(F&& f) && {
        if (has_value()) {
            return std::invoke(std::forward<F>(f),
                               std::move(std::get<VALUE>(std::move(value_))));
        }
        return *this;
    }
};

template <typename T, typename F>
    requires(result::is_optional<std::remove_cv_t<std::invoke_result_t<F, T&>>>)
constexpr auto and_then(std::optional<T> opt, F&& f) {
    using U = std::remove_cv_t<std::invoke_result_t<F, T&>>;
    if (opt.has_value()) {
        return std::invoke(std::forward<F>(f), *opt);
    } else {
        return U{};
    }
}

template <typename T, typename F>
constexpr auto map(std::optional<T> opt, F&& f) {
    using U = std::remove_cv_t<std::invoke_result_t<F, T&>>;
    if (opt.has_value()) {
        return std::optional{std::invoke(std::forward<F>(f), *opt)};
    }
    return std::optional<U>{};
}
}  // namespace waxwing
