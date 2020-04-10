#include "common.h"
#include <vector>
#include <thread>
#include <chrono>
#include <mutex>

using namespace std::literals;

namespace {
	struct Promise;
	using Task = SimpleCoro<Promise>;
	using TaskHandle = coroutine_handle<Promise>;

	struct Executor {
		std::vector<Task> tasks;

		std::vector<coroutine_handle<>> needs_waking;
		std::mutex needs_waking_mutex;

		void spawn(Task task) {
			needs_waking.push_back(task.handle);
			tasks.push_back(std::move(task));
		}

		void wakeup(coroutine_handle<> handle) {
			std::unique_lock guard {needs_waking_mutex};
			needs_waking.push_back(handle);
		}

		void run() {			
			std::unique_lock guard {needs_waking_mutex};
			auto needs_waking = std::exchange(this->needs_waking, {});
			guard.unlock();

			for (auto handle : needs_waking) {
				if (!handle || handle.done()) {
					continue;
				} 

				handle.resume();
			}

			std::erase_if(tasks, [] (auto&& task) {
				return !task.handle || task.handle.done();
			});
		}
	};

	static Executor* g_executor;

	struct Promise {
		Task get_return_object() {
			return { TaskHandle::from_promise(*this) };
		}

		auto initial_suspend() { return suspend_always{}; }
		auto final_suspend() { return suspend_always{}; }
		void return_void() {}
		void unhandled_exception() { std::terminate(); }
	};


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
			std::thread([h, when=when] () mutable {
				std::this_thread::sleep_until(when);
				g_executor->wakeup(h);
			}).detach();
		}
		void await_resume() {}
	};



	Task blah(int x) {
		std::printf("[blah %d] begin\n", x);

		for (int i = 0; i < x; i++) {
			co_await BasicAwaitable{};
		}

		std::printf("[blah %d] end\n", x);
	}


	Task timed(int x) {
		std::printf("[timed %d] begin\n", x);

		co_await TimedAwaitable{1500ms * x};

		std::printf("[timed %d] end\n", x);
	}

}


void test_executor() {
    TestScope scope {"test_executor"};

    Executor executor;
    g_executor = &executor;

    executor.spawn(blah(4));
    executor.spawn(blah(1));
    executor.spawn(blah(3));

    executor.spawn(timed(1));
    executor.spawn(timed(2));
    executor.spawn(timed(3));

    while (!executor.tasks.empty()) {
    	executor.run();
    	std::puts("...");
    	std::this_thread::sleep_for(500ms);
    }
}