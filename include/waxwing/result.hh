#pragma once

#include <cstddef>
#include <functional>
#include <optional>
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
static constexpr bool is_optional = false;
template <typename T>
static constexpr bool is_optional<std::optional<T>> = true;

template <typename G>
static constexpr bool is_error = false;
template <typename G>
static constexpr bool is_error<Error<G>> = true;

template <typename U>
static constexpr bool is_result = false;
template <typename U, typename G>
static constexpr bool is_result<Result<U, G>> = true;
}  // namespace result

template <typename E>
class Error {
    template <typename T>
    static constexpr bool can_be_error =
        (std::is_object_v<T>)&&(!std::is_array_v<T>)&&(!std::is_const_v<T>)&&(
            !std::is_volatile_v<T>)&&(!std::is_reference_v<T>);

    static_assert(!result::is_error<E> && can_be_error<E>);

    E value_;

public:
    template <typename Err = E>
        requires(!result::is_error<Err>) && (std::is_constructible_v<E, Err>)
    explicit constexpr Error(Err&& err) : value_{std::forward<Err>(err)} {}

    [[nodiscard]] constexpr E& error() & {
        return value_;
    }
    [[nodiscard]] constexpr E const& error() const& {
        return value_;
    }
    [[nodiscard]] constexpr E&& error() && {
        return value_;
    }
    [[nodiscard]] constexpr E const&& error() const&& {
        return value_;
    }
};

template <typename E>
Error(E) -> Error<E>;

template <typename T, typename E>
class Result {
    static_assert(!result::is_result<T> && !result::is_error<E>);

    static constexpr size_t VALUE = 0;
    static constexpr size_t ERROR = 1;

    bool has_value_;

    using value_type = std::conditional_t<std::is_void_v<T>, std::monostate, T>;
    using error_type = E;
    std::variant<value_type, error_type> value_;

public:
    constexpr Result()
        requires(std::is_void_v<T> || std::is_default_constructible_v<T>)
        : has_value_{true}, value_{} {}

    template <typename U = T>
        requires(!result::is_result<U>) && (std::is_constructible_v<T, U>)
    explicit(!std::is_convertible_v<U, T>) constexpr Result(U&& value)
        : has_value_{true}, value_{std::forward<U>(value)} {}

    template <typename G = E>
        requires(std::is_constructible_v<E, G>)
    explicit(!std::is_convertible_v<const G&, E>) constexpr Result(
        const Error<G>& error)
        : has_value_{false}, value_{std::forward<const G&>(error.error())} {}

    template <typename G = E>
        requires(std::is_constructible_v<E, G>)
    explicit(!std::is_convertible_v<G, E>) constexpr Result(Error<G>&& error)
        : has_value_{false}, value_{std::forward<G>(error.error())} {}

    template <typename U = T, typename G = E>
        requires(!result::is_result<U>) &&
                (std::is_constructible_v<T, const U&>) &&
                (std::is_constructible_v<E, const G&>)
    constexpr Result(const Result<U, G>& res) : has_value_{res.has_value()} {
        if (has_value_) {
            std::get<VALUE>(value_) = res.error();
        } else {
            std::get<ERROR>(value_) = res.value();
        }
    }

    template <typename U = T, typename G = E>
        requires(!result::is_result<U>) && (std::is_constructible_v<T, U>) &&
                (std::is_constructible_v<E, G>)
    constexpr Result(Result<U, G>&& res) : has_value_{res.has_value()} {
        if (has_value_) {
            std::get<VALUE>(value_) = std::move(res.error());
        } else {
            std::get<ERROR>(value_) = std::move(res.value());
        }
    }

    constexpr bool has_value() const {
        return has_value_;
    }

    constexpr bool has_error() const {
        return !has_value_;
    }

    constexpr operator bool() const {
        return has_value();
    }

    [[nodiscard]] constexpr const T* operator->() const noexcept
        requires(!std::is_void_v<T>)
    {
        return &std::get<VALUE>(value_);
    }
    [[nodiscard]] constexpr T* operator->() noexcept
        requires(!std::is_void_v<T>)
    {
        return &std::get<VALUE>(value_);
    }

    [[nodiscard]] constexpr const value_type& operator*() const& noexcept {
        return std::get<VALUE>(value_);
    }
    [[nodiscard]] constexpr value_type& operator*() & noexcept {
        return std::get<VALUE>(value_);
    }
    [[nodiscard]] constexpr const value_type&& operator*() const&& noexcept {
        return std::get<VALUE>(value_);
    }
    [[nodiscard]] constexpr value_type&& operator*() && noexcept {
        return std::get<VALUE>(value_);
    }

    [[nodiscard]] constexpr value_type& value() &
        requires(!std::is_void_v<T>)
    {
        return std::get<VALUE>(value_);
    }
    [[nodiscard]] constexpr value_type const& value() const&
        requires(!std::is_void_v<T>)
    {
        return std::get<VALUE>(value_);
    }
    [[nodiscard]] constexpr value_type&& value() &&
        requires(!std::is_void_v<T>)
    {
        return std::get<VALUE>(value_);
    }
    [[nodiscard]] constexpr value_type const&& value() const&&
        requires(!std::is_void_v<T>)
    {
        return std::get<VALUE>(value_);
    }

    template <typename U = T>
    [[nodiscard]] constexpr value_type value_or(U&& default_value) const&
        requires(!std::is_void_v<T>) && (std::is_constructible_v<T, U>)
    {
        if (has_value_) {
            return std::get<VALUE>(value_);
        }
        return value_type{std::forward<U>(default_value)};
    }

    template <typename U = T>
    [[nodiscard]] constexpr value_type value_or(U&& default_value) &&
        requires(!std::is_void_v<T>) && (std::is_constructible_v<T, U>)
    {
        if (has_value_) {
            return std::get<VALUE>(value_);
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
        return std::get<ERROR>(value_);
    }
    [[nodiscard]] constexpr error_type&& error() &&
        requires(!std::is_void_v<E>)
    {
        return std::get<ERROR>(value_);
    }
    [[nodiscard]] constexpr error_type const&& error() const&&
        requires(!std::is_void_v<E>)
    {
        return std::get<ERROR>(value_);
    }

    template <typename F, typename U>
    constexpr Result<U, E> map(F&& f) const& {
        if (has_value()) {
            return Result{std::invoke(std::forward<F>(f), value())};
        }
        return *this;
    }

    template <typename F, typename U>
    constexpr Result<U, E> map(F&& f) && {
        if (has_value()) {
            return Result{std::invoke(std::forward<F>(f), value())};
        }
        return *this;
    }
};

template <typename T, typename F>
    requires(result::is_optional<std::remove_cv_t<std::invoke_result_t<F, T&>>>)
constexpr auto and_then(std::optional<T> opt, F&& f) {
    using U = std::remove_cv_t<std::invoke_result_t<F, T&>>;
    if (opt.has_value()) {
        return std::invoke(f, *opt);
    } else {
        return U{};
    }
}

template <typename T, typename F>
constexpr auto map(std::optional<T> opt, F f) {
    using U = std::remove_cv_t<std::invoke_result_t<F, T&>>;
    if (opt.has_value()) {
        return std::optional{std::invoke(f, opt.value())};
    }
    return std::optional<U>{};
}
}  // namespace waxwing
