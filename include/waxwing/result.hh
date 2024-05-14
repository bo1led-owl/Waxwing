#pragma once

#include <functional>
#include <optional>
#include <stdexcept>
#include <type_traits>

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
        requires(!result::is_error<Err>) && (std::is_constructible_v<E, Err>)
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

    static_assert(!result::is_result<T> && !result::is_error<T> &&
                  result::can_be_value<T> && !result::is_result<E> &&
                  !result::is_error<E> && result::can_be_error<E>);

    bool has_value_;

    using value_type = std::conditional_t<std::is_void_v<T>, char, T>;
    using error_type = E;
    union {
        value_type value_;
        error_type error_;
    };

public:
    ~Result() {
        if (has_value_) {
            if constexpr (std::is_destructible_v<value_type>) {
                value_.~value_type();
            }
        } else {
            if constexpr (std::is_destructible_v<error_type>) {
                error_.~error_type();
            }
        }
    }

    constexpr Result()
        requires(std::is_void_v<T> || std::is_default_constructible_v<T>)
        : has_value_{true}, value_{} {}

    constexpr Result(const Result&) = default;
    constexpr Result(Result&&) = default;

    constexpr Result& operator=(const Result&) = default;
    constexpr Result& operator=(Result&&) = default;

    template <typename U = T>
        requires(!std::is_same_v<std::remove_cvref_t<U>, Result>) &&
                    (!result::is_result<U>) && (!result::is_error<U>) &&
                    (std::is_constructible_v<T, U>)
    constexpr explicit(!std::is_convertible_v<U, T>) Result(U&& value)
        : has_value_{true}, value_{std::forward<U>(value)} {}

    template <typename G = E>
        requires(std::is_constructible_v<E, G>) && (!result::is_error<G>)
    constexpr explicit(!std::is_convertible_v<G, E>)
        Result(const Error<G>& error)
        : has_value_{false}, error_{error.error()} {}

    template <typename G = E>
        requires(std::is_constructible_v<E, G>)
    constexpr explicit(!std::is_convertible_v<G, E>) Result(Error<G>&& error)
        : has_value_{false}, error_{std::move(error).error()} {}

    template <typename U = T, typename G = E>
        requires(std::is_constructible_v<T, const U&>) &&
                (std::is_constructible_v<E, const G&>)
    constexpr explicit(!std::is_convertible_v<U, T> ||
                       !std::is_convertible_v<G, E>)
        Result(const Result<U, G>& res)
        : has_value_{res.has_value()} {
        if (has_value_) {
            std::construct_at(std::addressof(value_), res.value_);
        } else {
            std::construct_at(std::addressof(error_), res.error_);
        }
    }

    template <typename U = T, typename G = E>
        requires(std::is_constructible_v<T, U>) &&
                (std::is_constructible_v<E, G>)
    constexpr explicit(!std::is_convertible_v<U, T> ||
                       !std::is_convertible_v<G, E>) Result(Result<U, G>&& res)
        : has_value_{res.has_value()} {
        if (has_value_) {
            std::construct_at(std::addressof(value_), std::move(res).value_);
        } else {
            std::construct_at(std::addressof(error_), std::move(res).error_);
        }
    }

    constexpr bool has_value() const { return has_value_; }

    constexpr bool has_error() const { return !has_value_; }

    constexpr operator bool() const { return has_value(); }

    constexpr operator value_type() const = delete;
    constexpr operator error_type() const
        requires(!std::is_same_v<value_type, error_type>)
    = delete;

    [[nodiscard]] constexpr const T* operator->() const noexcept
        requires(!std::is_void_v<T>)
    {
        return std::addressof(value_);
    }
    [[nodiscard]] constexpr T* operator->() noexcept
        requires(!std::is_void_v<T>)
    {
        return std::addressof(value_);
    }

    [[nodiscard]] constexpr const value_type& operator*() const& noexcept {
        return value_;
    }
    [[nodiscard]] constexpr value_type& operator*() & noexcept {
        return value_;
    }
    [[nodiscard]] constexpr const value_type&& operator*() const&& noexcept {
        return std::move(value_);
    }
    [[nodiscard]] constexpr value_type&& operator*() && noexcept {
        return std::move(value_);
    }

    [[nodiscard]] constexpr value_type& value() &
        requires(!std::is_void_v<T>)
    {
        return value_;
    }
    [[nodiscard]] constexpr value_type const& value() const&
        requires(!std::is_void_v<T>)
    {
        if (!has_value_) [[unlikely]] {
            throw std::logic_error(
                "Accessing `value` of `Result` which doesn't have a value");
        }
        return value_;
    }
    [[nodiscard]] constexpr value_type&& value() &&
        requires(!std::is_void_v<T>)
    {
        if (!has_value_) [[unlikely]] {
            throw std::logic_error(
                "Accessing `value` of `Result` which doesn't have a value");
        }
        return std::move(value_);
    }
    [[nodiscard]] constexpr value_type const&& value() const&&
        requires(!std::is_void_v<T>)
    {
        if (!has_value_) [[unlikely]] {
            throw std::logic_error(
                "Accessing `value` of `Result` which doesn't have a value");
        }
        return std::move(value_);
    }

    template <typename U = T>
    [[nodiscard]] constexpr value_type value_or(U&& default_value) const&
        requires(!std::is_void_v<T>) && (std::is_constructible_v<T, U>)
    {
        if (has_value_) {
            return value_;
        }
        return value_type{std::forward<U>(default_value)};
    }

    template <typename U = T>
    [[nodiscard]] constexpr value_type value_or(U&& default_value) &&
        requires(!std::is_void_v<T>) && (std::is_constructible_v<T, U>)
    {
        if (has_value_) {
            return std::move(value_);
        }
        return value_type{std::forward<U>(default_value)};
    }

    [[nodiscard]] constexpr error_type& error() &
        requires(!std::is_void_v<E>)
    {
        return error_;
    }
    [[nodiscard]] constexpr error_type const& error() const&
        requires(!std::is_void_v<E>)
    {
        if (has_value_) [[unlikely]] {
            throw std::logic_error(
                "Accessing `error` of `Result` which has a value");
        }
        return error_;
    }
    [[nodiscard]] constexpr error_type&& error() &&
        requires(!std::is_void_v<E>)
    {
        if (has_value_) [[unlikely]] {
            throw std::logic_error(
                "Accessing `error` of `Result` which has a value");
        }
        return std::move(error_);
    }
    [[nodiscard]] constexpr error_type const&& error() const&&
        requires(!std::is_void_v<E>)
    {
        if (has_value_) [[unlikely]] {
            throw std::logic_error(
                "Accessing `error` of `Result` which has a value");
        }
        return std::move(error_);
    }

    template <typename F, typename U>
    constexpr Result<U, E> map(F&& f) const& {
        if (has_value()) {
            return Result{std::invoke(std::forward<F>(f), value_)};
        }
        return *this;
    }

    template <typename F, typename U>
    constexpr Result<U, E> map(F&& f) && {
        if (has_value()) {
            return Result{std::invoke(std::forward<F>(f), std::move(value_))};
        }
        return *this;
    }

    template <typename F, typename U>
    constexpr Result<U, E> and_then(F&& f) const& {
        if (has_value()) {
            return std::invoke(std::forward<F>(f), value_);
        }
        return *this;
    }

    template <typename F, typename U>
    constexpr Result<U, E> and_then(F&& f) && {
        if (has_value()) {
            return std::invoke(std::forward<F>(f), std::move(value_));
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
