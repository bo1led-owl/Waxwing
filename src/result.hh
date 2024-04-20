#pragma once

#include <cstddef>
#include <variant>

namespace http {
template <typename E>
class Error {
    E value_;

public:
    explicit Error(E err) : value_{std::move(err)} {}
    E& error() {
        return value_;
    }
};

template <typename T = void, typename E = void>
class Result {
    // static_assert(std::is_default_constructible_v<T> || std::is_void_v<T>,
    //               "T must be void or default constructible");

    static constexpr size_t SUCCESS = 0;
    static constexpr size_t ERROR = 1;

    bool ok_;

    using value_type = std::conditional_t<std::is_void_v<T>, std::monostate, T>;
    using error_type = std::conditional_t<std::is_void_v<E>, std::monostate, E>;
    std::variant<value_type, error_type> value_;

public:
    Result()
        requires(std::is_void_v<T>)
        : ok_{true}, value_{} {}

    template <typename U = T>
        requires(!std::is_void_v<U> && std::is_same_v<U, T> &&
                 std::is_move_constructible_v<U>)
    Result(U value) : ok_{true}, value_{std::move(value)} {}

    template <typename U = T>
        requires(!std::is_void_v<U> && std::is_same_v<U, T> &&
                 !std::is_move_constructible_v<U>)
    Result(U value) : ok_{true}, value_{value} {}

    template <typename U>
        requires((std::is_convertible_v<U, T> || std::is_same_v<U, T>) &&
                 !std::is_same_v<U, Result<U, E>> && !std::is_void_v<T> &&
                 std::is_move_constructible_v<U>)
    Result(Result<U> res)
        : ok_{res.has_value()}, value_{std::move(res.value())} {}

    template <typename U>
        requires((std::is_convertible_v<U, T> || std::is_same_v<U, T>) &&
                 !std::is_same_v<U, Result<U, E>> && !std::is_void_v<T> &&
                 !std::is_move_constructible_v<U>)
    Result(Result<U> res) : ok_{res.has_value()}, value_{res.value()} {}

    template <typename U>
        requires(std::is_void_v<U> && std::is_void_v<T>)
    Result(Result<U> res) : ok_{res.has_value()}, value_{} {}

    template <typename U>
        requires(std::is_convertible_v<U, E>)
    Result(Error<U> error)
        : ok_{false}, value_{static_cast<E>(error.error())} {}

    bool has_value() const {
        return ok_;
    }

    operator bool() const {
        return has_value();
    }

    value_type& value() &
        requires(!std::is_void_v<T>)
    {
        return std::get<SUCCESS>(value_);
    }

    const value_type& value() const&
        requires(!std::is_void_v<T>)
    {
        return std::get<SUCCESS>(value_);
    }

    value_type&& value() &&
        requires(!std::is_void_v<T>)
    {
        return std::get<SUCCESS>(value_);
    }

    const value_type&& value() const&&
        requires(!std::is_void_v<T>)
    {
        return std::get<SUCCESS>(value_);
    }

    error_type& error() &
        requires(!std::is_void_v<E>)
    {
        return std::get<ERROR>(value_);
    }
    const error_type& error() const&
        requires(!std::is_void_v<E>)
    {
        return std::get<ERROR>(value_);
    }
    error_type&& error() &&
        requires(!std::is_void_v<E>)
    {
        return std::get<ERROR>(value_);
    }
    const error_type&& error() const&&
        requires(!std::is_void_v<E>)
    {
        return std::get<ERROR>(value_);
    }
};

template <typename T>
Result<T> make_result(const T& value) {
    return Result<T>{value};
}

template <typename T>
Result<T> make_result(T&& value) {
    return Result<T>{std::move(value)};
}
}  // namespace http
