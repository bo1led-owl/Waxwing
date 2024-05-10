#pragma once

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>
#include <utility>
#include <vector>

#include "movable_function.hh"

namespace waxwing {
namespace internal {
namespace concurrency {
using Task = MovableFunction<void()>;

class TaskQueue {
    std::queue<Task> repr_;
    std::mutex mut_;
    std::condition_variable cond_;
    bool done_ = false;

    bool is_empty() const {
        return repr_.empty();
    }

    bool is_done() const {
        return done_;
    }

public:
    TaskQueue() : mut_{}, cond_{} {}

    bool try_push(Task&& f) {
        {
            const std::unique_lock<std::mutex> lock{mut_};
            if (!lock) {
                return false;
            }
            repr_.emplace(std::move(f));
        }
        cond_.notify_one();
        return true;
    }

    void push(Task&& f) {
        {
            const std::lock_guard<std::mutex> lock{mut_};
            repr_.emplace(std::move(f));
        }
        cond_.notify_one();
    }

    bool try_pop(Task& result) {
        std::unique_lock<std::mutex> lock{mut_};
        cond_.wait(lock, [this]() { return !is_empty() || is_done(); });
        if (!lock || is_empty()) {
            return false;
        }

        result = std::move(repr_.front());
        repr_.pop();
        return true;
    }

    bool pop(Task& result) {
        std::unique_lock<std::mutex> lock{mut_};
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
            std::unique_lock<std::mutex> lock{mut_};
            done_ = true;
        }
        cond_.notify_all();
    }

    bool is_empty_and_done() {
        std::unique_lock<std::mutex> l{mut_};
        return is_done() && is_empty();
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
                const unsigned cur = (assigned_queue + i) % NUM_THREADS;
                if (queues_[cur].try_pop(f)) {
                    break;
                }
            }

            if (!f && !queues_[assigned_queue].pop(f)) {
                break;
            } else {
                f();
            }
        }
    }

public:
    ThreadPool() {
        for (unsigned int i = 0; i != NUM_THREADS; ++i) {
            threads_.emplace_back([&, i]() { thread_func(i); });
        }
    }

    ~ThreadPool() {
        for (auto& queue : queues_) {
            queue.done();
        }
    }

    void async(MovableFunction<void()>&& f) {
        const unsigned int cur = cur_queue_index_++;

        for (unsigned i = 0; i != NUM_THREADS; ++i) {
            if (queues_[(cur + i) % NUM_THREADS].try_push(std::move(f))) {
                return;
            }
        }

        queues_[cur & NUM_THREADS].push(std::move(f));
    }
};
}  // namespace concurrency
}  // namespace internal
}  // namespace waxwing
