#include "thread_pool.hh"

namespace waxwing::internal::concurrency {
bool TaskQueue::is_empty() const {
    return repr_.empty();
}

bool TaskQueue::is_done() const {
    return done_;
}

bool TaskQueue::try_push(Task&& f) {
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

void TaskQueue::wait_and_push(Task&& f) {
    {
        const std::lock_guard<std::mutex> lock{mut_};
        repr_.emplace(std::move(f));
    }
    cond_.notify_one();
}

bool TaskQueue::try_pop(Task& result) {
    std::unique_lock<std::mutex> lock{mut_};
    cond_.wait(lock, [this]() { return !is_empty() || is_done(); });
    if (!lock || is_empty()) {
        return false;
    }

    result = std::move(repr_.front());
    repr_.pop();
    return true;
}

bool TaskQueue::wait_and_pop(Task& result) {
    std::unique_lock<std::mutex> lock{mut_};
    cond_.wait(lock, [this]() { return !is_empty() || is_done(); });
    if (is_empty()) {
        return false;
    }

    result = std::move(repr_.front());
    repr_.pop();
    return true;
}

void TaskQueue::done() {
    {
        const std::lock_guard<std::mutex> lock{mut_};
        done_ = true;
    }
    cond_.notify_all();
}

// bool TaskQueue::is_empty_and_done() {
//     const std::lock_guard<std::mutex> lock{mut_};
//     return is_done() && is_empty();
// }

void ThreadPool::thread_func(const unsigned int assigned_queue) {
    for (;;) {
        Task f;

        for (unsigned int i = 0; i != num_threads_; ++i) {
            const unsigned cur = (assigned_queue + i) % num_threads_;
            if (queues_[cur].try_pop(f)) {
                break;
            }
        }

        if ((!f && !queues_[assigned_queue].wait_and_pop(f))) {
            break;
        } else {
            f();
        }
    }
}

ThreadPool::ThreadPool(const unsigned int threads)
    : num_threads_{threads}, queues_{threads} {
    for (unsigned int i = 0; i != num_threads_; ++i) {
        threads_.emplace_back([&, i]() { thread_func(i); });
    }
}

ThreadPool::~ThreadPool() {
    for (auto& queue : queues_) {
        queue.done();
    }
}

void ThreadPool::async(MovableFunction<void()>&& f) {
    const unsigned int cur = cur_queue_index_++;

    for (unsigned i = 0; i != num_threads_; ++i) {
        if (queues_[(cur + i) % num_threads_].try_push(std::move(f))) {
            return;
        }
    }

    queues_[cur & num_threads_].wait_and_push(std::move(f));
}
}  // namespace waxwing::internal::concurrency
