#include "common.h"

namespace {
    template<class T = void>
    struct Generator {
        struct promise_type;
        using Handle = coroutine_handle<promise_type>;

        std::optional<T> next() {
            if (handle && !handle.done()) {
                handle.resume();
                return handle.promise().value;
            }

            return std::nullopt;
        }

        Generator(Generator const&) = delete;
        Generator(Generator&& o) : handle{std::exchange(o.handle, nullptr)} {}
        ~Generator() { if (handle) handle.destroy(); }


    private:
        struct Done {};

    public:
        auto begin() {
            struct iter {
                std::optional<T> value;
                Generator gen;

                iter& operator++() { value = gen.next(); return *this; }
                T& operator*() { return value.value(); }

                bool operator!=(Done) const { return !gen.handle.done(); }
                bool operator==(Done) const { return gen.handle.done(); }
            };

            return iter{next(), std::move(*this)};
        }

        static auto end() { return Done {}; }

    private:
        Generator(Handle handle) : handle{handle} {}
        Handle handle;
    };


    template<class T>
    struct Generator<T>::promise_type {
        std::optional<T> value {std::nullopt};

        Generator get_return_object() { return Generator{Handle::from_promise(*this)}; }
        auto initial_suspend() { return std::experimental::suspend_always{}; }
        auto final_suspend() { return std::experimental::suspend_always{}; }
        auto yield_value(T t) {
            value = std::move(t);
            return std::experimental::suspend_always{};
        }

        void return_void() {
            value.reset();
        }

        void unhandled_exception() {
            std::puts("[gen ] Unhandled exception!");
            std::terminate();
        }
    };



    Generator<int> count(int x) {
        for (int i = 0; i < x; i++) {
            co_yield i;
        }
    }

    Generator<std::string> blah() {
        for (auto v : count(5)) {
            co_yield std::to_string(v * v);
        }
    }
}

void test_generator() {
    TestScope scope {"test_generator"};

    for (auto value : blah()) {
        std::printf("'%s'\n", value.data());
    }
}