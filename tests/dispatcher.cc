#include "waxwing/dispatcher.hh"

#include <gtest/gtest.h>

#include <atomic>
#include <mutex>
#include <thread>
#include <vector>

#include "waxwing/async.hh"

namespace {
using waxwing::async::Dispatcher;
using waxwing::async::Task;

TEST(ThreadPool, SingleProducer) {
    constexpr const int TASKS = 128;
    std::atomic<int> c = 0;

    auto add_c = [&c](Dispatcher& disp) -> Task<> {
        c += 1;
        co_return;
    };

    std::vector<Task<>> tasks;
    {
        Dispatcher disp;

        for (int i = 0; i < TASKS; ++i) {
            tasks.emplace_back(add_c(disp));
            disp.async(tasks.back().handle());
        }
        while (c < TASKS) {
            std::this_thread::yield();
        }
        for (int i = 0; i < TASKS; ++i) {
            EXPECT_TRUE(tasks[i].handle().done());
        }
    }
    EXPECT_EQ(c, TASKS);
}

TEST(ThreadPool, SingleProducerNestedCoroutines) {
    constexpr const int TASKS = 128;
    std::atomic<int> c = 0;

    auto foo = [](Dispatcher& disp) -> Task<int> { co_return 1; };

    auto add_c = [&c, &foo](Dispatcher& disp) -> Task<> {
        int a = co_await foo(disp);
        c += a;
    };

    std::vector<Task<>> tasks;
    {
        Dispatcher disp;

        for (int i = 0; i < TASKS; ++i) {
            tasks.emplace_back(add_c(disp));
            disp.async(tasks.back().handle());
        }
        while (c < TASKS) {
            std::this_thread::yield();
        }
        for (int i = 0; i < TASKS; ++i) {
            EXPECT_TRUE(tasks[i].handle().done());
        }
    }
    EXPECT_EQ(c, TASKS);
}

TEST(ThreadPool, MultipleProducers) {
    constexpr const int TASKS = 1024;
    constexpr const int CONSUMERS = 4;
    constexpr const int PRODUCERS = 4;

    ASSERT_EQ(TASKS % PRODUCERS, 0);

    std::atomic<int> consumed = 0;
    std::atomic<int> produced = 0;

    auto add_c = [&consumed](Dispatcher& disp) -> Task<> {
        consumed += 1;
        co_return;
    };

    std::mutex mut;
    std::vector<Task<>> tasks;

    {
        Dispatcher disp{CONSUMERS};

        std::vector<std::thread> producers;
        producers.reserve(PRODUCERS);

        for (int i = 0; i < PRODUCERS; ++i) {
            producers.emplace_back([&]() {
                while (produced < TASKS) {
                    std::lock_guard<std::mutex> l{mut};
                    tasks.emplace_back(add_c(disp));
                    disp.async(tasks.back().handle());
                    produced += 1;
                }
            });
        }

        while (produced < TASKS && consumed < TASKS) {
            std::this_thread::yield();
        }
        for (int i = 0; i < CONSUMERS; ++i) {
            EXPECT_TRUE(tasks[i].handle().done());
        }
        for (auto& prod : producers) {
            prod.join();
        }
    }

    EXPECT_EQ(produced, consumed);
}

TEST(ThreadPool, MultipleProducersNestedCoroutines) {
    constexpr const int TASKS = 1024;
    constexpr const int CONSUMERS = 4;
    constexpr const int PRODUCERS = 4;

    ASSERT_TRUE(TASKS % PRODUCERS == 0);

    std::atomic<int> consumed = 0;
    std::atomic<int> produced = 0;

    auto foo = [](Dispatcher& disp) -> Task<int> { co_return 1; };

    auto add_c = [&consumed, &foo](Dispatcher& disp) -> Task<> {
        consumed += co_await foo(disp);
    };

    std::mutex mut;
    std::vector<Task<>> tasks;
    tasks.reserve(TASKS);

    {
        Dispatcher disp{CONSUMERS};

        std::vector<std::thread> producers;
        producers.reserve(PRODUCERS);

        for (int i = 0; i < PRODUCERS; ++i) {
            producers.emplace_back([&]() {
                while (produced < TASKS) {
                    std::lock_guard<std::mutex> l{mut};
                    tasks.emplace_back(add_c(disp));
                    disp.async(tasks.back().handle());
                    produced += 1;
                }
            });
        }

        while (produced < TASKS && consumed < TASKS) {
            std::this_thread::yield();
        }
        for (int i = 0; i < CONSUMERS; ++i) {
            EXPECT_TRUE(tasks[i].handle().done());
        }
        for (auto& prod : producers) {
            prod.join();
        }
    }

    EXPECT_EQ(produced, consumed);
}
}  // namespace
