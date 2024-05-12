#pragma once

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

#include "movable_function.hh"

namespace waxwing::internal::concurrency {
using Task = MovableFunction<void()>;

class TaskQueue final {
    std::queue<Task> repr_;
    std::mutex mut_;
    std::condition_variable cond_;
    bool done_ = false;

    bool is_empty() const;
    bool is_done() const;

public:
    TaskQueue() = default;

    bool try_push(Task&& f);
    void push(Task&& f);
    bool try_pop(Task& result);
    bool pop(Task& result);
    void done();
    bool is_empty_and_done();
};

class ThreadPool final {
    const unsigned num_threads_;
    std::vector<TaskQueue> queues_;
    std::vector<std::jthread> threads_;
    std::atomic<unsigned int> cur_queue_index_ = 0;

    void thread_func(unsigned int assigned_queue);

public:
    ThreadPool(unsigned int threads = std::thread::hardware_concurrency());
    ~ThreadPool();
    void async(MovableFunction<void()>&& f);
};
}  // namespace waxwing::internal::concurrency
