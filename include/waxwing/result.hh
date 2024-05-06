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
    explicit Error(Err&& err) : value_{std::forward<Err>(err)} {}

    [[nodiscard]] E& error() & {
        return value_;
    }
    [[nodiscard]] const E& error() const& {
        return value_;
    }
    [[nodiscard]] E&& error() && {
        return value_;
    }
    [[nodiscard]] const E&& error() const&& {
        return value_;
    }
};

template <typename E>
Error(E) -> Error<E>;

template <typename T, typename E>
class Result {
    static_assert(!result::is_result<T> && !result::is_error<E>);

    static constexpr size_t SUCCESS = 0;
    static constexpr size_t ERROR = 1;

    bool has_value_;

    using value_type = std::conditional_t<std::is_void_v<T>, std::monostate, T>;
    using error_type = E;
    std::variant<value_type, error_type> value_;

public:
    Result()
        requires(std::is_void_v<T> || std::is_default_constructible_v<T>)
        : has_value_{true}, value_{} {}

    template <typename U = T>
        requires(!result::is_result<U>) && (std::is_constructible_v<T, U>)
    explicit(!std::is_convertible_v<U, T>) Result(U&& value)
        : has_value_{true}, value_{std::forward<U>(value)} {}

    template <typename G = E>
        requires(std::is_constructible_v<E, G>)
    explicit(!std::is_convertible_v<const G&, E>) Result(const Error<G>& error)
        : has_value_{false}, value_{std::forward<const G&>(error.error())} {}

    template <typename G = E>
        requires(std::is_constructible_v<E, G>)
    explicit(!std::is_convertible_v<G, E>) Result(Error<G>&& error)
        : has_value_{false}, value_{std::forward<G>(error.error())} {}

    template <typename U = T, typename G = E>
        requires(!result::is_result<U>) &&
                (std::is_constructible_v<T, const U&>) &&
                (std::is_constructible_v<E, const G&>)
    Result(const Result<U, G>& res) : has_value_{res.has_value()} {
        if (has_value_) {
            std::get<SUCCESS>(value_) = res.error();
        } else {
            std::get<ERROR>(value_) = res.value();
        }
    }

    template <typename U = T, typename G = E>
        requires(!result::is_result<U>) && (std::is_constructible_v<T, U>) &&
                (std::is_constructible_v<E, G>)
    Result(Result<U, G>&& res) : has_value_{res.has_value()} {
        if (has_value_) {
            std::get<SUCCESS>(value_) = std::move(res.error());
        } else {
            std::get<ERROR>(value_) = std::move(res.value());
        }
    }

    bool has_value() const {
        return has_value_;
    }

    bool has_error() const {
        return !has_value_;
    }

    operator bool() const {
        return has_value();
    }

    template <typename F, typename U>
    Result<U, E> map(F&& f) const& {
        if (has_value()) {
            return Result{std::invoke(std::forward<F>(f), value())};
        }
        return *this;
    }

    template <typename F, typename U>
    Result<U, E> map(F&& f) && {
        if (has_value()) {
            return Result{std::invoke(std::forward<F>(f), value())};
        }
        return *this;
    }

    [[nodiscard]] value_type& value() &
        requires(!std::is_void_v<T>)
    {
        return std::get<SUCCESS>(value_);
    }
    [[nodiscard]] const value_type& value() const&
        requires(!std::is_void_v<T>)
    {
        return std::get<SUCCESS>(value_);
    }
    [[nodiscard]] value_type&& value() &&
        requires(!std::is_void_v<T>)
    {
        return std::get<SUCCESS>(value_);
    }
    [[nodiscard]] const value_type&& value() const&&
        requires(!std::is_void_v<T>)
    {
        return std::get<SUCCESS>(value_);
    }

    template <typename U = T>
    [[nodiscard]] value_type value_or(U&& default_value) const&
        requires(!std::is_void_v<T>) && (std::is_constructible_v<T, U>)
    {
        if (has_value_) {
            return std::get<SUCCESS>(value_);
        }
        return value_type{std::forward<U>(default_value)};
    }

    template <typename U = T>
    [[nodiscard]] value_type value_or(U&& default_value) &&
        requires(!std::is_void_v<T>) && (std::is_constructible_v<T, U>)
    {
        if (has_value_) {
            return std::get<SUCCESS>(value_);
        }
        return value_type{std::forward<U>(default_value)};
    }

    [[nodiscard]] error_type& error() &
        requires(!std::is_void_v<E>)
    {
        return std::get<ERROR>(value_);
    }
    [[nodiscard]] const error_type& error() const&
        requires(!std::is_void_v<E>)
    {
        return std::get<ERROR>(value_);
    }
    [[nodiscard]] error_type&& error() &&
        requires(!std::is_void_v<E>)
    {
        return std::get<ERROR>(value_);
    }
    [[nodiscard]] const error_type&& error() const&&
        requires(!std::is_void_v<E>)
    {
        return std::get<ERROR>(value_);
    }
};

template <
    typename T, typename F,
    typename U = std::remove_cvref_t<std::invoke_result_t<F, T&>>::value_type>
std::optional<U> and_then(std::optional<T> opt, F&& f) {
    if (opt.has_value()) {
        return std::invoke(std::forward<F>(f), opt.value());
    }
    return std::nullopt;
}

template <typename T, typename F>
auto map(std::optional<T> opt, F f) {
    if (opt.has_value()) {
        return std::optional{std::invoke(f, opt.value())};
    }
    return std::nullopt;
}
}  
