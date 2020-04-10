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



template<class Promise>
struct SimpleCoro {
	using promise_type = Promise;

	SimpleCoro(coroutine_handle<Promise> h) : handle{h} {};
	SimpleCoro(SimpleCoro&& o) : handle{std::exchange(o.handle, {})} {} 

	SimpleCoro& operator=(SimpleCoro&& o) {
		if (handle) {
			handle.destroy();
		}

		handle = std::exchange(o.handle, {});
		return *this;
	}

	~SimpleCoro() {
		if (handle) {
			handle.destroy();
		}
	}

	bool resume() {
		if (handle && !handle.done()) {
			handle();
			return true;
		}

		return false;
	}

	coroutine_handle<Promise> handle;
};

