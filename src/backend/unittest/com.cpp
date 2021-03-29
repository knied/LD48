////////////////////////////////////////////////////////////////////////////////

#include "../com.hpp"
#include <gtest/gtest.h>
#include <algorithm>

////////////////////////////////////////////////////////////////////////////////

namespace {
struct TestCtx {
  void resume(char c = 'X') {
    _c = c;
    if (_handle && !_handle.done()) {
      _handle.resume();
    }
  }

  auto operator co_await() & noexcept {
    struct awaitable {
      awaitable(char& c, std::experimental::coroutine_handle<>& handle)
        : _c(c), _handle(handle) {}
      ~awaitable() { _handle = nullptr; }
      constexpr bool await_ready() noexcept { return false; }
      void await_suspend(std::experimental::coroutine_handle<> handle) {
        _handle = handle;
      }
      char await_resume() {
        return _c;
      }
      char& _c;
      std::experimental::coroutine_handle<>& _handle;
    };
    return awaitable{ _c, _handle };
  }
private:
  char _c = 0;
  std::experimental::coroutine_handle<> _handle;
};

struct TestSocket {
  coro::task<std::size_t>
  async_read(TestCtx& ctx, char* buffer, std::size_t count) {
    auto toCopy = std::min(count, (std::size_t)10);
    for (std::size_t i = 0; i < toCopy; ++i) {
      auto c = co_await ctx;
      if (c == 0) {
        co_return 0;
      }
      *(buffer++) = c;
    }
    co_return toCopy;
  }
  coro::task<std::size_t>
  async_write(TestCtx& ctx, char const* buffer, std::size_t count) {
    auto toCopy = std::min(count, (std::size_t)10);
    for (std::size_t i = 0; i < toCopy; ++i) {
      auto c = co_await ctx;
      if (c == 0) {
        co_return 0;
      }
      _written += buffer[i];
    }
    co_return toCopy;
  }
  std::string _written;
};

using test_channel = com::channel<TestCtx, TestSocket>;

coro::sync_task<void>
pullOneByOne(test_channel& channel, std::string& out) {
  for co_await (auto c : channel.async_char_stream()) {
    out += c;
  }
}

coro::sync_task<bool>
pullBuffer(test_channel& channel, std::size_t n, std::string& out) {
  out.resize(n);
  co_return co_await channel.async_read(out.data(), n);
}

coro::sync_task<bool>
pushBuffer(test_channel& channel, std::string const& in) {
  co_return co_await channel.async_write(in.data(), in.size());
}
}

TEST(com, async_char_stream) {
  TestCtx ctx;
  TestSocket socket;
  test_channel c(ctx, socket);

  std::string str;
  {
    auto task = pullOneByOne(c, str);
    task.start();
    ctx.resume('h');
    ctx.resume('e');
    ctx.resume('l');
    ctx.resume('l');
    ctx.resume('o');
  }
  ctx.resume('X');
  EXPECT_EQ(str, "hello");
}

TEST(com, async_char_stream_closed) {
  TestCtx ctx;
  TestSocket socket;
  test_channel c(ctx, socket);

  std::string str;
  auto task = pullOneByOne(c, str);
  task.start();
  EXPECT_FALSE(task.done());
  ctx.resume('h');
  ctx.resume('e');
  ctx.resume('l');
  EXPECT_FALSE(task.done());
  ctx.resume(0);
  EXPECT_TRUE(task.done());
  ctx.resume('l');
  ctx.resume('o');
  EXPECT_EQ(str, "hel");
}

TEST(com, async_read) {
  TestCtx ctx;
  TestSocket socket;
  test_channel c(ctx, socket);

  std::string str;
  auto task = pullBuffer(c, 10, str);
  task.start();
  EXPECT_FALSE(task.done());
  ctx.resume('h');
  ctx.resume('e');
  ctx.resume('l');
  ctx.resume('l');
  ctx.resume('o');
  ctx.resume(',');
  ctx.resume(' ');
  ctx.resume('w');
  ctx.resume('o');
  EXPECT_FALSE(task.done());
  ctx.resume('r');
  EXPECT_TRUE(task.done());
  ctx.resume('l');
  ctx.resume('d');
  ctx.resume('!');
  EXPECT_EQ(str, "hello, wor");
  EXPECT_TRUE(task.result());
}

TEST(com, async_read_closed) {
  TestCtx ctx;
  TestSocket socket;
  test_channel c(ctx, socket);

  std::string str;
  auto task = pullBuffer(c, 10, str);
  task.start();
  EXPECT_FALSE(task.done());
  ctx.resume('h');
  ctx.resume('e');
  ctx.resume('l');
  ctx.resume('l');
  ctx.resume('o');
  ctx.resume(0);
  EXPECT_TRUE(task.done());
  ctx.resume(',');
  ctx.resume(' ');
  ctx.resume('w');
  ctx.resume('o');
  ctx.resume('r');
  ctx.resume('l');
  ctx.resume('d');
  ctx.resume('!');
  EXPECT_FALSE(task.result());
}

TEST(com, async_write) {
  TestCtx ctx;
  TestSocket socket;
  test_channel c(ctx, socket);

  std::string str = "Hello, world!";
  auto task = pushBuffer(c, str);
  task.start();
  EXPECT_FALSE(task.done());
  ctx.resume();
  ctx.resume();
  ctx.resume();
  ctx.resume();
  ctx.resume();
  ctx.resume();
  ctx.resume();
  ctx.resume();
  ctx.resume();
  ctx.resume();
  ctx.resume();
  ctx.resume();
  ctx.resume();
  EXPECT_TRUE(task.done());
  EXPECT_TRUE(task.result());
  EXPECT_EQ(socket._written, "Hello, world!");
  ctx.resume();
  EXPECT_EQ(socket._written, "Hello, world!");
}

TEST(com, async_write_close) {
  TestCtx ctx;
  TestSocket socket;
  test_channel c(ctx, socket);

  std::string str = "Hello, world!";
  auto task = pushBuffer(c, str);
  task.start();
  EXPECT_FALSE(task.done());
  ctx.resume();
  ctx.resume();
  ctx.resume();
  ctx.resume();
  ctx.resume();
  ctx.resume();
  ctx.resume(0);
  EXPECT_TRUE(task.done());
  EXPECT_FALSE(task.result());
  ctx.resume();
  ctx.resume();
  ctx.resume();
  ctx.resume();
  ctx.resume();
  ctx.resume();
  ctx.resume();
  EXPECT_EQ(socket._written, "Hello,");
}

////////////////////////////////////////////////////////////////////////////////
