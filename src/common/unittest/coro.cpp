////////////////////////////////////////////////////////////////////////////////

#include "../coro.hpp"
#include <gtest/gtest.h>

////////////////////////////////////////////////////////////////////////////////

namespace {
struct Awaitable {
  constexpr bool await_ready() const { return false; }
  void await_suspend(std::experimental::coroutine_handle<> handle) {
    _handle = handle;
  }
  void await_resume() {}
  void resume() {
    if (_handle) {
      _handle.resume();
    }
  }
private:
  std::experimental::coroutine_handle<> _handle;
};
}

////////////////////////////////////////////////////////////////////////////////

coro::sync_task<void> basicVoid() { co_return; }
TEST(coro, task_basic_void) {
  auto t = basicVoid();
  t.start();
  t.result();
  EXPECT_TRUE(t.done());
}

////////////////////////////////////////////////////////////////////////////////

coro::sync_task<int> basicInt() { co_return 42; }
TEST(coro, task_basic_int) {
  auto t = basicInt();
  t.start();
  EXPECT_EQ(42, t.result());
  EXPECT_TRUE(t.done());
}

////////////////////////////////////////////////////////////////////////////////

coro::sync_task<void> exceptionVoid() {
  throw std::runtime_error("void_exception");
  co_return;
}
TEST(coro, task_exception_void) {
  auto t = exceptionVoid();
  t.start();
  auto didThrow = false;
  try {
    t.result();
  } catch (std::runtime_error& ex) {
    EXPECT_STREQ("void_exception", ex.what());
    didThrow = true;
  }
  EXPECT_TRUE(t.done());
  EXPECT_TRUE(didThrow);
}

////////////////////////////////////////////////////////////////////////////////

coro::sync_task<int> exceptionInt() {
  throw std::runtime_error("int_exception");
  co_return 42;
}
TEST(coro, task_exception_int) {
  auto t = exceptionInt();
  t.start();
  auto didThrow = false;
  try {
    t.result();
  } catch (std::runtime_error& ex) {
    EXPECT_STREQ("int_exception", ex.what());
    didThrow = true;
  }
  EXPECT_TRUE(t.done());
  EXPECT_TRUE(didThrow);
}

////////////////////////////////////////////////////////////////////////////////

coro::sync_task<void> awaitVoid(Awaitable& a) {
  co_await a;
  co_await a;
}
TEST(coro, task_await_void) {
  Awaitable a;
  auto t = awaitVoid(a);
  t.start();
  EXPECT_FALSE(t.done());
  a.resume();
  EXPECT_FALSE(t.done());
  a.resume();
  EXPECT_TRUE(t.done());
  t.result();
}

////////////////////////////////////////////////////////////////////////////////

coro::sync_task<int> awaitInt(Awaitable& a, int value) {
  co_await a;
  co_return value;
}
TEST(coro, task_await_int) {
  Awaitable a;
  auto t = awaitInt(a, 42);
  t.start();
  EXPECT_FALSE(t.done());
  a.resume();
  EXPECT_TRUE(t.done());
  EXPECT_EQ(42, t.result());
}

////////////////////////////////////////////////////////////////////////////////

TEST(coro, task_then) {
  Awaitable a;
  auto t0 = awaitInt(a, 42);
  auto t1 = awaitVoid(a);
  auto t2 = awaitInt(a, 23);
  t0.start()
    .then(t1)
    .then(t2);
  EXPECT_FALSE(t0.done());
  EXPECT_FALSE(t1.done());
  EXPECT_FALSE(t2.done());
  a.resume();
  EXPECT_TRUE(t0.done());
  EXPECT_FALSE(t1.done());
  EXPECT_FALSE(t2.done());
  a.resume();
  EXPECT_TRUE(t0.done());
  EXPECT_FALSE(t1.done());
  EXPECT_FALSE(t2.done());
  a.resume();
  EXPECT_TRUE(t0.done());
  EXPECT_TRUE(t1.done());
  EXPECT_FALSE(t2.done());
  a.resume();
  EXPECT_TRUE(t0.done());
  EXPECT_TRUE(t1.done());
  EXPECT_TRUE(t2.done());
  EXPECT_EQ(42, t0.result());
  EXPECT_EQ(23, t2.result());
}

////////////////////////////////////////////////////////////////////////////////

namespace {
struct Scope {
  Scope(bool& alive) : _alive(alive) { _alive = true; }
  ~Scope() { _alive = false; }
  bool& _alive;
};
}
coro::sync_task<void> lifetime(Awaitable& a, bool& b0, bool& b1) {
  {
    Scope s0(b0);
    co_await a;
  }
  Scope s1(b1);
  co_await a;
}
TEST(coro, task_lifetime) {
  Awaitable a;
  bool b0 = false;
  bool b1 = false;
  {
    auto t = lifetime(a, b0, b1);
    t.start();
    EXPECT_TRUE(b0);
    EXPECT_FALSE(b1);
    EXPECT_FALSE(t.done());
    a.resume();
    EXPECT_FALSE(b0);
    EXPECT_TRUE(b1);
    EXPECT_FALSE(t.done());
  }
  EXPECT_FALSE(b0);
  EXPECT_FALSE(b1);
}

////////////////////////////////////////////////////////////////////////////////

TEST(coro, task_movable) {
  Awaitable a;
  bool b0 = false;
  bool b1 = false;
  auto t0 = lifetime(a, b0, b1);
  t0.start();
  {
    auto t1 = std::move(t0);
    EXPECT_TRUE(b0);
    EXPECT_FALSE(b1);
    EXPECT_FALSE(t1.done());
    a.resume();
    EXPECT_FALSE(b0);
    EXPECT_TRUE(b1);
    EXPECT_FALSE(t1.done());
  }
  EXPECT_FALSE(b0);
  EXPECT_FALSE(b1);
}

////////////////////////////////////////////////////////////////////////////////

template<typename awaitable_type>
coro::async_task<int> otherAwaitableTask(awaitable_type& a) {
  co_await a;
  co_return 17;
}
template<typename awaitable_type>
coro::async_task<int> awaitableTask(awaitable_type& a) {
  auto v = co_await otherAwaitableTask(a);
  co_return v * 23;
}
template<typename awaitable_type>
coro::sync_task<int> entryTask(awaitable_type& a) {
  auto v = co_await a;
  co_return v + 42;
}
TEST(coro, async_task) {
  Awaitable a;
  auto t0 = awaitableTask(a);
  auto t1 = entryTask(t0);
  t1.start();
  EXPECT_FALSE(t0.done());
  EXPECT_FALSE(t1.done());
  a.resume();
  EXPECT_TRUE(t0.done());
  EXPECT_TRUE(t1.done());
  EXPECT_EQ((17 * 23) + 42, t1.result());
}

////////////////////////////////////////////////////////////////////////////////

coro::generator<int> generateInts(std::vector<int> numbers) {
  for (auto i : numbers) {
    co_yield i;
  }
  co_return;
}
TEST(coro, generator) {
  auto ints = generateInts({1, 42});
  auto it = ints.begin();
  EXPECT_EQ(1, *it);
  ++it;
  EXPECT_EQ(42, *it);
  ++it;
  EXPECT_EQ(ints.end(), it);
}

////////////////////////////////////////////////////////////////////////////////

TEST(coro, generatorLoop) {
  std::vector<int> numbers{23,12,4};
  auto it = numbers.begin();
  for (auto i : generateInts(numbers)) {
    EXPECT_EQ(*it++, i);
  }
}

////////////////////////////////////////////////////////////////////////////////

template<typename awaitable_type>
coro::async_generator<int> generateAsyncInts(awaitable_type& a, std::vector<int> numbers) {
  int c = 0;
  for (auto i : numbers) {
    if ((c++) % 2 == 0) {
      co_await a;
    }
    co_yield i;
  }
}
template<typename awaitable_type>
coro::sync_task<int> asyncSum(awaitable_type& a, std::vector<int> numbers) {
  int sum = 0;
  for co_await (auto i : generateAsyncInts(a, numbers)) {
      sum += i;
    }
  co_return sum;
}

TEST(coro, async_generator_await) {
  Awaitable a;
  auto t = asyncSum(a, {2,4,7,5,3});
  t.start();
  EXPECT_FALSE(t.done());
  a.resume();
  EXPECT_FALSE(t.done());
  a.resume();
  EXPECT_FALSE(t.done());
  a.resume();
  EXPECT_TRUE(t.done());
  EXPECT_EQ(2 + 4 + 7 + 5 + 3, t.result());
}

////////////////////////////////////////////////////////////////////////////////

coro::async_generator<int> generateAsyncInts(std::vector<int> numbers) {
  for (auto i : numbers) {
    co_yield i;
  }
}
coro::sync_task<int> asyncSum(std::vector<int> numbers) {
  int sum = 0;
  for co_await (auto i : generateAsyncInts(numbers)) {
      sum += i;
    }
  co_return sum;
}

TEST(coro, async_generator_no_await) {
  auto t = asyncSum({2,4,7});
  t.start();
  EXPECT_TRUE(t.done());
  EXPECT_EQ(2 + 4 + 7, t.result());
}

////////////////////////////////////////////////////////////////////////////////

template<typename awaitable_type>
coro::async_generator<int> innerGenerator(awaitable_type& a) {
  co_yield 1;
  co_await a;
  co_yield 2;
}

coro::generator<int> outerGenerator() {
  Awaitable a;
  auto inner = innerGenerator(a);
  for (auto it = inner.begin(); it != inner.end(); ++it) {
    auto t = [&it]() -> coro::sync_task<void> {
      co_await it;
    }();
    t.start();
    while (!t.done()) {
      a.resume();
    }
    if (it != inner.end()) {
      co_yield *it;
    }
  }
}

TEST(coro, mixed_generator) {
  auto outer = outerGenerator();
  auto it = outer.begin();
  EXPECT_FALSE(it == outer.end());
  EXPECT_EQ(1, *it);
  ++it;
  EXPECT_FALSE(it == outer.end());
  EXPECT_EQ(2, *it);
  ++it;
  EXPECT_TRUE(it == outer.end());

  int expected = 1;
  for (auto i : outerGenerator()) {
    EXPECT_EQ(i, expected++);
  }
}

////////////////////////////////////////////////////////////////////////////////
