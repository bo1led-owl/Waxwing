#pragma once

#include <fmt/core.h>
#include <spdlog/spdlog.h>

#include <atomic>
#include <cassert>
#include <condition_variable>
#include <coroutine>
#include <mutex>
#include <queue>
#include <thread>
#include <utility>
#include <vector>

namespace waxwing::async {
namespace internal {
template <typename T>
class TaskQueue final {
    std::queue<T> repr_;
    std::mutex mutex_;
    std::condition_variable cond_;
    bool done_ = false;

    /// Not thread-safe
    bool is_empty() const noexcept { return repr_.empty(); }
    /// Not thread-safe
    bool is_done() const noexcept { return done_; }

public:
    TaskQueue() = default;

    bool try_push(T f) {
        {
            const std::unique_lock<std::mutex> lock{mutex_};
            if (!lock) {
                return false;
            }
            repr_.emplace(std::move(f));
        }
        cond_.notify_one();
        return true;
    }

    void wait_and_push(T f) {
        {
            const std::lock_guard<std::mutex> lock{mutex_};
            repr_.emplace(std::move(f));
        }
        cond_.notify_one();
    }

    bool try_pop(T& result) {
        std::unique_lock<std::mutex> lock{mutex_};
        cond_.wait(lock, [this]() { return !is_empty() || is_done(); });
        if (!lock || is_empty()) {
            return false;
        }

        result = std::move(repr_.front());
        repr_.pop();
        return true;
    }

    bool wait_and_pop(T& result) {
        std::unique_lock<std::mutex> lock{mutex_};
        cond_.wait(lock, [this]() { return !is_empty() || is_done(); });
        if (is_empty()) {
            return false;
        }

        result = std::move(repr_.front());
        repr_.pop();
        return true;
    }

    void done() {
        {
            const std::lock_guard<std::mutex> l{mutex_};
            done_ = true;
        }
        cond_.notify_all();
    }
};
}  // namespace internal

class Dispatcher final {
    const unsigned num_threads_;
    std::vector<internal::TaskQueue<std::coroutine_handle<>>> queues_;
    std::vector<std::jthread> threads_;
    std::atomic<unsigned int> cur_queue_index_ = 0;

    void thread_func(const unsigned int assigned_queue) {
        for (;;) {
            std::coroutine_handle<> f;

            for (unsigned int i = 0; i != num_threads_; ++i) {
                const unsigned cur = (assigned_queue + i) % num_threads_;
                if (queues_[cur].try_pop(f)) {
                    break;
                }
            }

            if ((!f && !queues_[assigned_queue].wait_and_pop(f))) {
                break;
            } else if (!f.done()) {
                spdlog::debug("t{}: executing {}", assigned_queue, f.address());
                f.resume();
            }
        }
    }

public:
    Dispatcher(const unsigned int threads = std::thread::hardware_concurrency())
        : num_threads_{threads}, queues_{threads} {
        for (unsigned int i = 0; i != num_threads_; ++i) {
            threads_.emplace_back([&, i]() { thread_func(i); });
        }
    }

    ~Dispatcher() {
        for (auto& queue : queues_) {
            queue.done();
        }
    }

    void async(std::coroutine_handle<> f) {
        assert(f && !f.done());
        const unsigned int cur = cur_queue_index_++;

        for (unsigned i = 0; i != num_threads_; ++i) {
            if (queues_[(cur + i) % num_threads_].try_push(f)) {
                // spdlog::debug("added {}", f.address());
                return;
            }
        }

        queues_[cur % num_threads_].wait_and_push(f);
        // spdlog::debug("added {}", f.address());
    }
};
}  // namespace waxwing::async
