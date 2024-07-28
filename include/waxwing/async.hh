#pragma once

#include <spdlog/spdlog.h>

#include <cassert>
#include <coroutine>
#include <exception>
#include <optional>
#include <utility>

#include "waxwing/dispatcher.hh"
#include "waxwing/movable_function.hh"

namespace waxwing::async {
using waxwing::internal::MovableFunction;

template <typename T, typename R>
concept Awaitable = requires(T x) {
    { x.await_ready() } -> std::same_as<bool>;
    requires(
        requires(std::coroutine_handle<> h) {
            { x.await_suspend(h) } -> std::same_as<void>;
        } ||
        requires(std::coroutine_handle<> h) {
            { x.await_suspend(h) } -> std::same_as<bool>;
        } ||
        requires(std::coroutine_handle<> h) {
            { x.await_suspend(h) } -> std::same_as<std::coroutine_handle<>>;
        });
    { x.await_resume() } -> std::same_as<R>;
};

template <typename Task, typename T>
class Promise;

template <typename Task>
class Promise<Task, void> final {
    using coro_handle = std::coroutine_handle<Promise>;

    std::coroutine_handle<> precursor_;
    Dispatcher& disp_;
    MovableFunction<void()> deleter_{};

public:
    friend Task;

    template <typename... Args>
    Promise(Dispatcher& disp, Args&&...) : disp_{disp} {}

    template <typename U, typename... Args>
    Promise(U&, Dispatcher& disp, Args&&...) : disp_{disp} {}

    Task get_return_object() { return Task{coro_handle::from_promise(*this)}; }

    auto initial_suspend() { return std::suspend_always{}; }
    auto final_suspend() noexcept {
        class Awaiter final {
        public:
            Awaiter() = default;

            bool await_ready() const noexcept { return false; }

            void await_suspend(std::coroutine_handle<Promise> h) noexcept {
                auto precursor = h.promise().precursor_;
                if (precursor) {
                    h.promise().disp_.async(precursor);
                }

                auto& deleter = h.promise().deleter_;
                if (deleter) {
                    deleter();
                }
            }

            void await_resume() noexcept {}
        };

        return Awaiter{};
    }

    void return_void() {}
    void unhandled_exception() { std::terminate(); }

    void set_deleter(MovableFunction<void()> deleter) {
        deleter_ = std::move(deleter);
    }
    void set_precursor(std::coroutine_handle<> h) noexcept { precursor_ = h; }

    std::coroutine_handle<> get_precursor() const noexcept {
        return precursor_;
    }
};

template <typename Task, typename T>
class Promise final {
    using coro_handle = std::coroutine_handle<Promise>;

    std::coroutine_handle<> precursor_;
    std::optional<T> value_;
    Dispatcher& disp_;
    MovableFunction<void()> deleter_;

    Dispatcher& get_disp() noexcept { return disp_; }

public:
    friend Task;

    template <typename... Args>
    Promise(Dispatcher& disp, Args&&...) : disp_{disp} {}

    template <typename U, typename... Args>
    Promise(U&, Dispatcher& disp, Args&&...) : disp_{disp} {}

    Task get_return_object() { return Task{coro_handle::from_promise(*this)}; }

    auto initial_suspend() { return std::suspend_always{}; }
    auto final_suspend() noexcept {
        class Awaiter final {
        public:
            Awaiter() = default;

            bool await_ready() const noexcept { return false; }

            void await_suspend(std::coroutine_handle<Promise> h) noexcept {
                auto precursor = h.promise().precursor_;
                if (precursor) {
                    h.promise().disp_.async(precursor);
                }

                auto& deleter = h.promise().deleter_;
                if (deleter) {
                    deleter();
                }
            }

            void await_resume() noexcept {}
        };

        return Awaiter{};
    }

    void return_value(T val) { value_ = std::move(val); }
    void unhandled_exception() { std::terminate(); }

    void set_deleter(MovableFunction<void()> deleter) {
        deleter_ = std::move(deleter);
    }
    void set_precursor(std::coroutine_handle<> h) noexcept { precursor_ = h; }

    std::coroutine_handle<> get_precursor() const noexcept {
        return precursor_;
    }
    std::optional<T> value() noexcept { return std::move(value_); }
};

template <typename T = void>
struct [[nodiscard]] Task final {
    using promise_type = Promise<Task, T>;
    using coro_handle = std::coroutine_handle<promise_type>;

private:
    coro_handle handle_;

public:
    Task(std::coroutine_handle<promise_type> h) : handle_{h} {}
    ~Task() {
        if (handle_) {
            spdlog::info("~Task {}", handle_.address());
            handle_.destroy();
        }
    }

    Task(const Task&) = delete;
    Task& operator=(const Task&) = delete;

    Task(Task&& t) noexcept : handle_{std::exchange(t.handle_, nullptr)} {}
    Task& operator=(Task&& t) noexcept {
        std::swap(handle_, t.handle_);
        return *this;
    }

    std::coroutine_handle<promise_type> handle() const noexcept {
        return handle_;
    }

    bool done() const noexcept { return !handle_ || handle_.done(); }
    void set_deleter(MovableFunction<void()> deleter) {
        handle_.promise().set_deleter(std::move(deleter));
    }

    auto operator co_await() {
        class Awaiter {
            Dispatcher& disp_;
            coro_handle handle_;

        public:
            Awaiter(Dispatcher& disp, coro_handle handle)
                : disp_{disp}, handle_{handle} {
                assert(handle_);
            }
            bool await_ready() const noexcept { return false; }

            void await_suspend(std::coroutine_handle<> h) const noexcept {
                assert(h);
                assert(handle_);
                handle_.promise().set_precursor(h);
                disp_.async(handle_);
            }

            T await_resume() const noexcept {
                if constexpr (!std::is_void_v<T>) {
                    return std::move(handle_.promise().value().value());
                }
            }
        };

        return Awaiter{handle_.promise().get_disp(), handle_};
    }
};

// class AsyncMutex {
//     std::coroutine_handle<> awaiting_coroutine_{};
//     std::atomic<bool> locked_ = false;

// public:
//     AsyncMutex() = default;

//     bool is_locked() const noexcept { return locked_; }

//     auto lock() {
//         class Awaiter {
//             AsyncMutex& parent_;

//         public:
//             Awaiter(AsyncMutex& mut) : parent_{mut} {}

//             bool await_ready() noexcept { return false; }
//             bool await_suspend(std::coroutine_handle<> h) const noexcept
//             {
//                 if (!parent_.locked_) {
//                     parent_.locked_ = true;
//                     return false;
//                 }

//                 return true;
//             }

//             void await_resume() const noexcept {}
//         };

//         if (locked_) {}

//         return Awaiter{*this};
//     }
// };
}  // namespace waxwing::async
