#pragma once

#include <experimental/coroutine>
#include <optional>
#include <cstdio>
#include <cstring>
#include <numeric>

using std::experimental::coroutine_handle;
using std::experimental::suspend_always;
using std::experimental::suspend_never;


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
