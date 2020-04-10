#include <experimental/coroutine>
#include <cstdio>

// template<class T>
// struct Future {
//     bool await_ready() {
//         std::puts("[futr] ready");
//         return true;
//     }

//     void await_suspend(std::experimental::coroutine_handle<>) {
//         std::puts("[futr] suspend");
//     }

//     T await_resume() {
//         std::puts("[futr] resume");
//         return T{};
//     }
// };


// template<class Task>
// struct Promise {
//     using coro_handle = std::experimental::coroutine_handle<Promise<Task>>;

//     Task get_return_object() {
//         std::puts("[prom] get_return_object");
//         return coro_handle::from_promise(*this);
//     }

//     auto initial_suspend() {
//         std::puts("[prom] initial_suspend");
//         return std::experimental::suspend_always();
//     }

//     auto final_suspend() {
//         std::puts("[prom] final_suspend");
//         return std::experimental::suspend_always();
//     }

//     void return_void() { std::puts("[prom] return_void"); }
//     void unhandled_exception() {}
// };


// struct Task {
//     using promise_type = Promise<Task>;
//     using coro_handle = std::experimental::coroutine_handle<Promise<Task>>;
    
//     Task(coro_handle handle) : handle(handle) { assert(handle); }
//     Task(Task&) = delete;
//     Task(Task&&) = default;
//     ~Task() { handle.destroy(); }

//     bool resume() {
//         if (!handle.done()) {
//             handle.resume();
//         }

//         return !handle.done();
//     }

//     coro_handle handle;
// };


// Task async_fn() {
//     std::puts("[coro] Hello");
//     for (int i = 0; i < 3; i++) {
//         co_await std::experimental::suspend_always();
//         std::puts("[coro] Resumed");
//     }
// }
