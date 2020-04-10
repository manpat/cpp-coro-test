#include <experimental/coroutine>
#include <cstdio>
#include <thread>
#include <chrono>

#include "generator.h"
#include "simple_awaitable.h"


// https://isocpp.org/files/papers/N4663.pdf
// http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/n4775.pdf


int main() {
    test_generator();
    test_simple_awaitable();
}