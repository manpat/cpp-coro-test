#include "common.h"
#include <vector>
#include <mutex>
#include <array>

namespace {
    struct TaskPromise;
    using Task = SimpleCoro<TaskPromise>;

    struct Executor {
        std::vector<Task> m_owned_tasks;

        std::vector<coroutine_handle<>> m_needs_waking;
        std::vector<coroutine_handle<>> m_currently_waking;
        std::mutex needs_waking_mutex;

        void spawn(Task task) {
            m_needs_waking.push_back(task.handle());
            m_owned_tasks.push_back(std::move(task));
        }

        void wakeup(coroutine_handle<> handle) {
            std::unique_lock guard {needs_waking_mutex};
            m_needs_waking.push_back(handle);
        }

        bool run() {
            {
                std::unique_lock guard {needs_waking_mutex};
                std::swap(m_needs_waking, m_currently_waking);
            }

            for (auto handle : m_currently_waking) {
                if (!handle || handle.done()) {
                    continue;
                }

                handle.resume();
            }

            m_currently_waking.clear();

            std::erase_if(m_owned_tasks, [] (auto&& task) {
                return !task.handle() || task.handle().done();
            });

            return !m_owned_tasks.empty();
        }
    };

    static Executor* g_executor;

    struct TaskPromise {
        Task get_return_object() {
            return { coroutine_handle<TaskPromise>::from_promise(*this) };
        }

        auto initial_suspend() { return suspend_always{}; }
        auto final_suspend() { return suspend_always{}; }
        void return_void() {}
        void unhandled_exception() { std::terminate(); }
    };




    struct JoinCounter {
        JoinCounter(int c) : count{c} {}

        bool is_ready() const { return (bool) to_resume; }
        bool try_await(coroutine_handle<> h) {
            to_resume = h;
            return count > 0;
        }

        void notify_completed() {
            count--;
            if (count == 0) {
                g_executor->wakeup(to_resume);
            }
        }

        int count;
        coroutine_handle<> to_resume {};
    };

    struct JoinTask;

    struct JoinPromise {
        JoinTask get_return_object();

        auto initial_suspend() { return suspend_always{}; }
        auto final_suspend() {
            struct Awaitable {
                bool await_ready() { return false; }

                void await_suspend(coroutine_handle<JoinPromise> h) {
                    h.promise().counter->notify_completed();
                }

                void await_resume() {}
            };

            return Awaitable{};
        }
        void return_void() {}
        void unhandled_exception() { std::terminate(); }

        JoinCounter* counter {nullptr};
    };

    struct JoinTask {
        using promise_type = JoinPromise;

        JoinTask(coroutine_handle<JoinPromise> h) : handle{h} {}

        void start(JoinCounter* counter) {
            handle.promise()->counter = counter;
            handle.resume();
        }

        OwnedHandle<JoinPromise> handle;
    };

    JoinTask JoinPromise::get_return_object() {
        return { coroutine_handle<JoinPromise>::from_promise(*this) };
    }


    template<class Awaitable>
    JoinTask make_join_task(Awaitable&& awaitable) {
        co_await std::forward<Awaitable>(awaitable);
    }


    template<int N>
    struct JoinAwaitable {
        JoinAwaitable(std::array<JoinTask, N> ts)
            : tasks{std::move(ts)}
            , counter{N}
        {}

        bool await_ready() { return counter.is_ready(); }

        bool await_suspend(coroutine_handle<> h) {
            for (auto&& task : tasks) {
                task.start(&counter);
            }

            return counter.try_await(h);
        }

        void await_resume() {}

        std::array<JoinTask, N> tasks;
        JoinCounter counter;
    };

    template<class... Awaitable>
    auto join(Awaitable&&... awaitable) {
        return JoinAwaitable<sizeof...(Awaitable)> {{
            make_join_task(std::forward<Awaitable>(awaitable)) ...
        }};
    }




    struct BasicAwaitable {
        bool await_ready() { return false; }
        void await_suspend(coroutine_handle<> h) {
            g_executor->wakeup(h);
        }
        void await_resume() {}
    };

    struct TimedAwaitable {
        using clock = std::chrono::high_resolution_clock;

        clock::time_point when;

        TimedAwaitable(clock::duration d) : when{clock::now() + d} {}

        bool await_ready() { return when <= clock::now(); }
        void await_suspend(coroutine_handle<> h) {
            std::thread([h, when=this->when] () mutable {
                std::this_thread::sleep_until(when);
                g_executor->wakeup(h);
            }).detach();
        }
        void await_resume() {}
    };



    Task basic(int x) {
        std::printf("[basic %d] begin\n", x);

        for (int i = 0; i < x; i++) {
            co_await BasicAwaitable{};
        }

        std::printf("[basic %d] end\n", x);
    }


    Task timed(int x) {
        std::printf("[timed %d] begin\n", x);

        co_await TimedAwaitable{1500ms * x};

        std::printf("[timed %d] end\n", x);
    }


    Task counter() {
        std::puts("[counter] begin");

        std::puts("[counter] joining with no args");
        co_await join();

        std::puts("[counter] joining with three args");
        co_await join(BasicAwaitable{}, TimedAwaitable{500ms}, TimedAwaitable{1000ms});

        std::puts("[counter] end");
    }

}



void test_executor() {
    TestScope scope {"test_executor"};

    Executor executor;
    g_executor = &executor;

    executor.spawn(basic(3));

    executor.spawn(timed(0));
    executor.spawn(timed(2));

    executor.spawn(counter());

    while (executor.run()) {
        std::puts("...");
        std::this_thread::sleep_for(500ms);
    }
}