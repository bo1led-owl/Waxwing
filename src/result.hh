#pragma once

#include <cstddef>
#include <functional>
#include <type_traits>
#include <variant>

namespace http {
template <typename E>
class Error {
    template <typename G>
    static constexpr bool is_error =
        std::is_same_v<std::remove_cvref_t<G>, Error<G>>;
    template <typename T>
    static constexpr bool can_be_error =
        (std::is_object_v<E> && !std::is_array_v<E> && !std::is_const_v<E> &&
         !std::is_volatile_v<E> && !std::is_reference_v<E>);

    static_assert(!is_error<E> && can_be_error<E>);

    E value_;

public:
    explicit Error(E err) : value_{std::move(err)} {}

    template <typename Err = E>
        requires(!is_error<Err> && std::is_constructible_v<E, Err>)
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

template <typename T, typename E>
class Result {
    template <typename U>
    static constexpr bool is_result = std::is_same_v<U, Result>;

    static_assert(!is_result<T>);

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
        requires(!std::is_same_v<std::remove_cvref_t<U>, Result> &&
                 std::is_constructible_v<T, U>)
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
        requires(!is_result<U> && std::is_constructible_v<T, const U&> &&
                 std::is_constructible_v<E, const G&>)
    Result(const Result<U, G>& res) : has_value_{res.has_value()} {
        if (has_value()) {
            std::get<SUCCESS>(value_) = res.error();
        } else {
            std::get<ERROR>(value_) = res.value();
        }
    }

    template <typename U = T, typename G = E>
        requires(!is_result<U> && std::is_constructible_v<T, U> &&
                 std::is_constructible_v<E, G>)
    Result(Result<U, G>&& res) : has_value_{res.has_value()} {
        if (has_value()) {
            std::get<SUCCESS>(value_) = std::move(res.error());
        } else {
            std::get<ERROR>(value_) = std::move(res.value());
        }
    }

    bool has_value() const {
        return has_value_;
    }

    operator bool() const {
        return has_value();
    }

    template <typename F>
    Result<F, E> map(F&& f) const& {
        if (has_value()) {
            return Result{std::invoke(std::forward<F>(f), value())};
        }
        return *this;
    }

    template <typename F>
    Result<F, E> map(F&& f) && {
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

    [[nodiscard]] error_type& error() & {
        return std::get<ERROR>(value_);
    }
    [[nodiscard]] const error_type& error() const& {
        return std::get<ERROR>(value_);
    }
    [[nodiscard]] error_type&& error() && {
        return std::get<ERROR>(value_);
    }
    [[nodiscard]] const error_type&& error() const&& {
        return std::get<ERROR>(value_);
    }
};
}  // namespace http
