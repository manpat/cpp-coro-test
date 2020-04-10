#include "common.h"

struct SimpleAwaitable {
	const char* name = "await simple";

	bool await_ready() {
		std::printf("[%s] is_ready\n", name);
		return false;
	}
	void await_suspend(coroutine_handle<>) {
		std::printf("[%s] on_suspend\n", name);
	}
	const char* await_resume() {
		std::printf("[%s] on_resume\n", name);
		return name;
	}
};

// A non-awaitable, transformed into an awaitable w/ a co_await overload
struct IndirectAwaitable {};

auto operator co_await(IndirectAwaitable) {
	return SimpleAwaitable {"await indirect"};
}

// A non-awaitable, transformed into an awaitable w/ an await_transform overload in the context of a SimpleCoro
struct TransformAwaitable {};

// An awaitable that immediately transfers control to another coroutine
struct AsymmetricAwaitable {
	coroutine_handle<> next_handle;

	bool await_ready() {
		std::printf("[await asym] is_ready\n");
		return !next_handle || next_handle.done();
	}

	auto await_suspend(coroutine_handle<>) {
		std::printf("[await asym] on_suspend\n");
		return next_handle;
	}

	void await_resume() {
		std::printf("[await asym] on_resume???\n");
	}
};


template<class Promise>
struct Coro {
	using promise_type = Promise;

	~Coro() {
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


struct Promise {
	Promise() {
		std::puts("[promise simp] default constructor!");
	}
	~Promise() {
		std::puts("[promise simp] destroyed!");
	}

	void* operator new(std::size_t sz) {
		std::printf("[promise simp] NEW %lu\n", sz);
		return ::operator new(sz);
	}

	void operator delete(void* ptr) {
		std::printf("[promise simp] DELETE\n");
		::operator delete(ptr);
	}


	Coro<Promise> get_return_object() {
		return { coroutine_handle<Promise>::from_promise(*this) };
	}

	auto initial_suspend() { return suspend_always{}; }
	auto final_suspend() { return suspend_always{}; }
	void return_void() {}
	void unhandled_exception() { std::terminate(); }

	// If await_transform is defined, all co_awaits are transformed into `co_await ctx.await_transform(awaitable)`
	template<class T>
	auto await_transform(T&& t) {
		return std::forward<T>(t);
	}

	auto await_transform(TransformAwaitable) {
		return SimpleAwaitable {"await txform"};
	}
};


Coro<Promise> async_asymmetrical_awaitable(coroutine_handle<> next) {
	std::puts("[coro asym] start");
	std::printf("[coro asym] await '%s'\n", co_await SimpleAwaitable{});
	std::puts("[coro asym] transfer control...");
	co_await AsymmetricAwaitable{next};
	std::puts("[coro asym] end");
}

Coro<Promise> async_simple_awaitable() {
	std::puts("[coro simp] start");
	std::printf("[coro simp] await '%s'\n", co_await SimpleAwaitable{});
	std::printf("[coro simp] await '%s'\n", co_await IndirectAwaitable{});
	std::printf("[coro simp] await '%s'\n", co_await TransformAwaitable{});
	std::puts("[coro simp] end");
}


void test_simple_awaitable() {
    TestScope scope {"test_simple_awaitable"};

    auto coro3 = async_simple_awaitable();
    auto coro2 = async_asymmetrical_awaitable(coro3.handle);
    auto coro = async_asymmetrical_awaitable(coro2.handle);
    // auto coro = async_simple_awaitable();
    while (coro.resume()) {
    	std::puts("\n= suspend =\n");
    }

	std::puts("= coro expired =\n");

    while (coro2.resume()) {
    	std::puts("\n= suspend =\n");
    }

	std::puts("= coro expired =\n");

    while (coro3.resume()) {
    	std::puts("\n= suspend =\n");
    }

	std::puts("= coro expired =\n");
}