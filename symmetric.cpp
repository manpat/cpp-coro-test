#include "common.h"

namespace {
    template<class Yields>
    struct SymmetricCoroutinePromiseYieldBase;
    template<class Accepts>
    struct SymmetricCoroutinePromiseAcceptBase;

    template<class Yields, class Accepts>
    struct SymmetricCoroutinePromise;

    template<class Yields, class Accepts>
    struct SymmetricCoroutine {
        using promise_type = SymmetricCoroutinePromise<Yields, Accepts>;

        SymmetricCoroutine(coroutine_handle<promise_type> handle) : m_handle{handle} {}

        bool run() {
            if (m_handle.valid() && !m_handle.done()) {
                m_handle.resume();
                return true;
            }

            return false;
        }

        template<class TargetYields>
        void set_target(coroutine_handle<SymmetricCoroutinePromise<TargetYields, Yields>> next_coro) {
            m_handle.promise()->next_coro = next_coro;
        }

        coroutine_handle<promise_type> handle() { return m_handle.as_unowned(); }

    private:
        OwnedHandle<promise_type> m_handle;
    };



    template<class Promise>
    struct PassControlAwaitable {
        Promise* promise;

        bool await_ready() {
            // return promise->prev_coro && promise->prev_coro.promise().value;
            return false;
        }

        coroutine_handle<> await_suspend(coroutine_handle<> h) {
            if (promise->next_coro && !promise->next_coro.done()) {
                // :'(
                using NextPromise = SymmetricCoroutinePromise<int, typename Promise::YieldType>;
                auto next_handle_addr = promise->next_coro.address();
                auto next_handle = coroutine_handle<NextPromise>::from_address(next_handle_addr);

                next_handle.promise().prev_coro = h;
                return promise->next_coro;
            }

            return noop_coroutine();
        }

        std::optional<typename Promise::AcceptType> await_resume() {
            if (promise->prev_coro) {
                // :'(
                using PrevPromise = SymmetricCoroutinePromise<typename Promise::AcceptType, int>;
                auto prev_promise_addr = promise->prev_coro.address();
                auto prev_promise = coroutine_handle<PrevPromise>::from_address(prev_promise_addr);

                return std::move(prev_promise.promise().value);
            }

            return std::nullopt;
        }
    };


    template<class Yields, class Accepts>
    struct SymmetricCoroutinePromise {
        using YieldType = Yields;
        using AcceptType = Accepts;

        coroutine_handle<>              next_coro {};
        coroutine_handle<>              prev_coro {};
        std::optional<Yields>           value {std::nullopt};

        auto initial_suspend() { return std::experimental::suspend_always{}; }
        auto final_suspend() { return std::experimental::suspend_always{}; }

        void unhandled_exception() { std::terminate(); }

        void return_void() {
            this->value.reset();
        }

        auto yield_value(Yields t) {
            this->value = std::move(t);
            return PassControlAwaitable<SymmetricCoroutinePromise> { this };
        }

        SymmetricCoroutine<Yields, Accepts> get_return_object() {
            return { coroutine_handle<SymmetricCoroutinePromise>::from_promise(*this) };
        }
    };



    SymmetricCoroutine<std::string_view, int> get_input() {
        std::puts("[input] start");

        while (true) {
            char buf[256] {};

            std::printf("> ");

            if (!std::fgets(buf, 256, stdin)) {
                continue;
            }

            std::printf("[input] '%s'\n", buf);
            std::string_view input{buf};
            input.remove_suffix(1);

            if (!input.empty()) {
                co_yield input;
            } else {
                std::puts("[input] empty");
                break;
            }
        }

        std::puts("[input] end");
    }


    struct Token { std::string_view text; };

    SymmetricCoroutine<Token, std::string_view> tokenize(std::string_view delimiters) {
        std::puts("[tok] start");

        auto data = co_yield Token {};

        std::printf("[tok] first data %s\n", data? "valid" : "invalid");

        while (data) {
            auto delim = data->find_first_of(delimiters);
            // std::printf("[tok] %d\n", (int)delim);

            if (delim != data->npos) {
                data = co_yield Token { data->substr(0, delim+1) };
            } else {
                data = co_yield Token { *data };
            }
        }

        std::puts("[tok] end");
    }

    SymmetricCoroutine<int, Token> print_data() {
        std::puts("[parse] start");

        while (auto value = co_yield 0) {
            auto text = value->text;
            std::printf("[parse] token '%.*s'\n", (int) text.size(), text.data());
        }

        std::puts("[parse] end");
    }
}


void test_symmetric() {
    TestScope scope {"test_symmetric"};

    auto input_coro = get_input();
    auto tokenise_coro = tokenize(".!?");
    auto print_coro = print_data();

    print_coro.run();
    tokenise_coro.run();

    input_coro.set_target(tokenise_coro.handle());
    tokenise_coro.set_target(print_coro.handle());
    print_coro.set_target(input_coro.handle());

    while (input_coro.run()) {
        std::puts("YIELD");
        std::this_thread::sleep_for(500ms);
    }
}