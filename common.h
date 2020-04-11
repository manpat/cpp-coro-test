#pragma once

#include <experimental/coroutine>
#include <optional>
#include <cstdio>
#include <cstring>
#include <numeric>
#include <string>
#include <algorithm>

using std::experimental::coroutine_handle;
using std::experimental::suspend_always;
using std::experimental::suspend_never;
using std::experimental::noop_coroutine;


struct TestScope {
    int len;

    TestScope(const char* name) : len{(int)std::strlen(name)} {
        std::printf("=== %s ===\n", name);
    }

    ~TestScope() {
        constexpr const char equals[] {"==========================================================="};
        constexpr int equals_len = sizeof(equals);
        std::printf("====%.*s====\n\n", std::min(len, equals_len), equals);
    }
};


template<class Promise = void>
struct OwnedHandle {
    explicit OwnedHandle(coroutine_handle<Promise> h) : m_handle{h} {}
    OwnedHandle(OwnedHandle&& o) : m_handle{std::exchange(o.m_handle, {})} {}

    ~OwnedHandle() {
        if (m_handle) {
            m_handle.destroy();
        }
    }

    OwnedHandle& operator=(OwnedHandle&& o) {
        if (m_handle) {
            m_handle.destroy();
        }

        m_handle = std::exchange(o.m_handle, {});
        return *this;
    }

    bool valid() const { return (bool) m_handle; }
    bool done() const { return m_handle.done(); }
    void resume() { m_handle.resume(); }

    template<class P = Promise>
    auto promise() -> std::enable_if_t<!std::is_void_v<P>, P*> {
        if (m_handle) {
            return &m_handle.promise();
        } else {
            return nullptr;
        }
    }

    coroutine_handle<Promise> as_unowned() { return m_handle; }
    operator coroutine_handle<Promise>() { return m_handle; }
    operator coroutine_handle<void>() { return m_handle; }

    coroutine_handle<Promise> m_handle;
};


template<class Promise>
struct SimpleCoro {
    using promise_type = Promise;

    SimpleCoro(coroutine_handle<Promise> h) : m_handle{h} {};
    SimpleCoro(SimpleCoro&& o) = default; 
    ~SimpleCoro() = default;

    SimpleCoro& operator=(SimpleCoro&& o) = default;

    bool resume() {
        if (m_handle.valid() && !m_handle.done()) {
            m_handle.resume();
            return true;
        }

        return false;
    }

    Promise* promise() {
        return m_handle.promise();
    }

    coroutine_handle<Promise> handle() {
        return m_handle.as_unowned();
    }

private:
    OwnedHandle<Promise> m_handle;
};

