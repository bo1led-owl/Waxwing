#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <utility>
#include <vector>

namespace waxwing {
namespace internal {
namespace concurrency {
template <typename R, typename... Args>
struct FunctorContainerBase {
    virtual ~FunctorContainerBase() {}
    virtual R operator()(Args&&...) = 0;
};

template <typename F, typename R, typename... Args>
struct FunctorContainer final : FunctorContainerBase<R, Args...> {
    F f;

    FunctorContainer(F&& f) : f{std::forward<F>(f)} {}

    R operator()(Args&&... args) override {
        return f(std::forward<Args>(args)...);
    }
};

template <typename>
class MovableFunction;

template <typename R, typename... Args>
class MovableFunction<R(Args...)> final {
    std::unique_ptr<FunctorContainerBase<R, Args...>> f_;

public:
    MovableFunction() : f_{nullptr} {}
    ~MovableFunction() = default;

    template <typename F>
        requires(!std::is_same_v<F, MovableFunction<R(Args...)>>)
    MovableFunction(F&& f)
        : f_{std::make_unique<FunctorContainer<F, R, Args...>>(
              std::forward<F>(f))} {}

    MovableFunction(const MovableFunction&) = delete;
    MovableFunction& operator=(const MovableFunction& other) = delete;

    MovableFunction(MovableFunction&& other) noexcept {
        f_ = std::exchange(other.f_, nullptr);
    }

    MovableFunction& operator=(MovableFunction&& other) noexcept {
        std::swap(f_, other.f_);
        return *this;
    }

    R operator()(Args&&... args) const {
        return std::invoke(*f_, std::forward<Args>(args)...);
    }

    operator bool() const {
        return f_ != nullptr;
    }
};

using Task = MovableFunction<void()>;

class TaskQueue {
    std::queue<Task> repr_;
    std::mutex mut_;
    std::condition_variable cond_;

public:
    TaskQueue() {}

    bool empty() {
        return repr_.empty();
    }

    template <typename F>
    bool try_push(F&& x) {
        {
            const std::unique_lock<std::mutex> lock{mut_};
            if (!lock) {
                return false;
            }
            repr_.emplace(std::forward<F>(x));
        }
        cond_.notify_one();
        return true;
    }

    template <typename F>
    void push(F&& x) {
        {
            const std::unique_lock<std::mutex> lock{mut_};
            repr_.emplace(std::forward<F>(x));
        }
        cond_.notify_one();
    }

    bool try_pop(Task& result) {
        std::unique_lock<std::mutex> lock{mut_};
        cond_.wait(lock, [this]() { return !empty(); });
        if (!lock) {
            return false;
        }

        result = std::move(repr_.front());
        repr_.pop();
        return true;
    }
};

class ThreadPool {
    const unsigned int NUM_THREADS = std::thread::hardware_concurrency();

    std::vector<TaskQueue> queues_{NUM_THREADS};
    std::vector<std::jthread> threads_;
    std::atomic<unsigned int> cur_queue_index_ = 0;

    void thread_func(const unsigned int assigned_queue) {
        for (;;) {
            Task f;

            for (unsigned int i = 0; i != NUM_THREADS; ++i) {
                if (queues_[(assigned_queue + i) % NUM_THREADS].try_pop(f)) {
                    break;
                }
            }

            if (!f && !queues_[assigned_queue].try_pop(f)) {
                break;
            }
            f();
        }
    }

public:
    ThreadPool() {
        for (unsigned int i = 0; i != NUM_THREADS; ++i) {
            threads_.emplace_back([&, i]() { thread_func(i); });
        }
    }

    template <typename F>
    void async(F&& f) {
        const unsigned int cur = cur_queue_index_++;

        for (unsigned i = 0; i != NUM_THREADS; ++i) {
            if (queues_[(cur + i) % NUM_THREADS].try_push(std::forward<F>(f))) {
                return;
            }
        }

        queues_[cur & NUM_THREADS].push(std::forward<F>(f));
    }
};
}  // namespace concurrency
}  // namespace internal
}  // namespace waxwing
