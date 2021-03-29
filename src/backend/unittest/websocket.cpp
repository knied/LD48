////////////////////////////////////////////////////////////////////////////////

#include "../websocket.hpp"
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
      if (_closed) {
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
      (void)co_await ctx;
      if (_closed) {
        co_return 0;
      }
      _written.push_back(buffer[i]);
    }
    co_return toCopy;
  }
  void close() { _closed = true; }
  std::vector<char> _written;
  bool _closed = false;
};

using TestChannel = com::channel<TestCtx, TestSocket>;

coro::sync_task<bool>
pullFrame(TestChannel& channel, websocket::frame& out) {
  co_return co_await websocket::async_read(channel, out);
}
}

TEST(websocket, single_empty_frame) {
  TestCtx ctx;
  TestSocket socket;
  TestChannel channel(ctx, socket);

  websocket::frame frame;
  auto task = pullFrame(channel, frame);
  task.start();
  EXPECT_FALSE(task.done());
  ctx.resume(0x81); // fin=1 opcode=1
  ctx.resume(0x80); // mask=1 len=0
  ctx.resume(0x00); // mask[0]
  ctx.resume(0x00); // mask[1]
  ctx.resume(0x00); // mask[2]
  ctx.resume(0x00); // mask[3]
  EXPECT_TRUE(task.done());
  EXPECT_TRUE(task.result());
  
  EXPECT_TRUE(frame.fin);
  EXPECT_TRUE(frame.text());
  EXPECT_FALSE(frame.binary());
  std::vector<char> expected_data{};
  EXPECT_EQ(frame.data, expected_data);
}

TEST(websocket, single_text_frame) {
  TestCtx ctx;
  TestSocket socket;
  TestChannel channel(ctx, socket);

  websocket::frame frame;
  auto task = pullFrame(channel, frame);
  task.start();
  EXPECT_FALSE(task.done());
  ctx.resume(0x81); // fin=1 opcode=1
  ctx.resume(0x84); // mask=1 len=4
  ctx.resume(0x00); // mask[0]
  ctx.resume(0x00); // mask[1]
  ctx.resume(0x00); // mask[2]
  ctx.resume(0x00); // mask[3]
  ctx.resume('t');
  ctx.resume('e');
  ctx.resume('s');
  EXPECT_FALSE(task.done());
  ctx.resume('t');
  EXPECT_TRUE(task.done());
  EXPECT_TRUE(task.result());
  
  EXPECT_TRUE(frame.fin);
  EXPECT_TRUE(frame.text());
  EXPECT_FALSE(frame.binary());
  std::vector<char> expected_data{ 't', 'e', 's', 't' };
  EXPECT_EQ(frame.data, expected_data);
}

TEST(websocket, single_bin_frame) {
  TestCtx ctx;
  TestSocket socket;
  TestChannel channel(ctx, socket);

  websocket::frame frame;
  auto task = pullFrame(channel, frame);
  task.start();
  EXPECT_FALSE(task.done());
  ctx.resume(0x82); // fin=1 opcode=2
  ctx.resume(0x84); // mask=1 len=4
  ctx.resume(0x01); // mask[0]
  ctx.resume(0x20); // mask[1]
  ctx.resume(0x03); // mask[2]
  ctx.resume(0x40); // mask[3]
  ctx.resume(0x40);
  ctx.resume(0x03);
  ctx.resume(0x20);
  ctx.resume(0x01);
  EXPECT_TRUE(task.done());

  EXPECT_TRUE(task.result());
  EXPECT_TRUE(frame.fin);
  EXPECT_FALSE(frame.text());
  EXPECT_TRUE(frame.binary());
  std::vector<char> expected_data{ 0x41, 0x23, 0x23, 0x41 };
  EXPECT_EQ(frame.data, expected_data);
}

TEST(websocket, single_continuation_frame) {
  TestCtx ctx;
  TestSocket socket;
  TestChannel channel(ctx, socket);

  {
    websocket::frame frame;
    auto task = pullFrame(channel, frame);
    task.start();
    EXPECT_FALSE(task.done());
    ctx.resume(0x00); // fin=1 opcode=0
    ctx.resume(0x84); // mask=1 len=4
    ctx.resume(0x00); // mask[0]
    ctx.resume(0x00); // mask[1]
    ctx.resume(0x00); // mask[2]
    ctx.resume(0x00); // mask[3]
    ctx.resume('b');
    ctx.resume('l');
    ctx.resume('u');
    ctx.resume('b');
    EXPECT_TRUE(task.done());
    EXPECT_TRUE(task.result());

    EXPECT_FALSE(frame.fin);
    EXPECT_FALSE(frame.text());
    EXPECT_FALSE(frame.binary());
    std::vector<char> expected_data{ 'b', 'l', 'u', 'b' };
    EXPECT_EQ(frame.data, expected_data);
  }
}

TEST(websocket, single_text_frame_16bit) {
  TestCtx ctx;
  TestSocket socket;
  TestChannel channel(ctx, socket);

  {
    websocket::frame frame;
    auto task = pullFrame(channel, frame);
    task.start();
    EXPECT_FALSE(task.done());
    ctx.resume(0x00); // fin=1 opcode=0
    ctx.resume(0xFE); // mask=1 len=126
    ctx.resume(0x00); // len
    ctx.resume(0x04); // len
    ctx.resume(0x00); // mask[0]
    ctx.resume(0x00); // mask[1]
    ctx.resume(0x00); // mask[2]
    ctx.resume(0x00); // mask[3]
    ctx.resume('b');
    ctx.resume('l');
    ctx.resume('u');
    ctx.resume('b');
    EXPECT_TRUE(task.done());
    EXPECT_TRUE(task.result());

    EXPECT_FALSE(frame.fin);
    EXPECT_FALSE(frame.text());
    EXPECT_FALSE(frame.binary());
    std::vector<char> expected_data{ 'b', 'l', 'u', 'b' };
    EXPECT_EQ(frame.data, expected_data);
  }
}

TEST(websocket, single_text_frame_64bit) {
  TestCtx ctx;
  TestSocket socket;
  TestChannel channel(ctx, socket);

  {
    websocket::frame frame;
    auto task = pullFrame(channel, frame);
    task.start();
    EXPECT_FALSE(task.done());
    ctx.resume(0x00); // fin=1 opcode=0
    ctx.resume(0xFF); // mask=1 len=127
    ctx.resume(0x00); // len
    ctx.resume(0x00); // len
    ctx.resume(0x00); // len
    ctx.resume(0x00); // len
    ctx.resume(0x00); // len
    ctx.resume(0x00); // len
    ctx.resume(0x00); // len
    ctx.resume(0x04); // len
    ctx.resume(0x00); // mask[0]
    ctx.resume(0x00); // mask[1]
    ctx.resume(0x00); // mask[2]
    ctx.resume(0x00); // mask[3]
    ctx.resume('b');
    ctx.resume('l');
    ctx.resume('u');
    ctx.resume('b');
    EXPECT_TRUE(task.done());
    EXPECT_TRUE(task.result());

    EXPECT_FALSE(frame.fin);
    EXPECT_FALSE(frame.text());
    EXPECT_FALSE(frame.binary());
    std::vector<char> expected_data{ 'b', 'l', 'u', 'b' };
    EXPECT_EQ(frame.data, expected_data);
  }
}

namespace {
coro::sync_task<bool>
frameSequence(TestChannel& channel, std::vector<websocket::frame>& frames) {
  auto stream = websocket::stream(channel);
  for co_await (auto f : stream) {
    frames.push_back(std::move(f));
  }
  co_return true;
}
}

TEST(websocket, sequence_text_frame) {
  TestCtx ctx;
  TestSocket socket;
  TestChannel channel(ctx, socket);

  std::vector<websocket::frame> frames;
  auto task = frameSequence(channel, frames);
  task.start();
  EXPECT_FALSE(task.done());
  ctx.resume(0x01); // fin=0 opcode=1
  ctx.resume(0x84); // mask=1 len=4
  ctx.resume(0x00); // mask[0]
  ctx.resume(0x00); // mask[1]
  ctx.resume(0x00); // mask[2]
  ctx.resume(0x00); // mask[3]
  ctx.resume('t');
  ctx.resume('e');
  ctx.resume('s');
  ctx.resume('t');
  EXPECT_FALSE(task.done());
  ctx.resume(0x81); // fin=1 opcode=1
  ctx.resume(0x84); // mask=1 len=4
  ctx.resume(0x00); // mask[0]
  ctx.resume(0x00); // mask[1]
  ctx.resume(0x00); // mask[2]
  ctx.resume(0x00); // mask[3]
  ctx.resume('b');
  ctx.resume('l');
  ctx.resume('u');
  ctx.resume('b');
  socket.close();
  ctx.resume();
  EXPECT_TRUE(task.done());
  EXPECT_TRUE(task.result());
  ASSERT_EQ(frames.size(), 2ull);

  {
    EXPECT_FALSE(frames[0].fin);
    EXPECT_TRUE(frames[0].text());
    EXPECT_FALSE(frames[0].binary());
    std::vector<char> expected_data{ 't', 'e', 's', 't' };
    EXPECT_EQ(frames[0].data, expected_data);
  }
  {
    EXPECT_TRUE(frames[1].fin);
    EXPECT_TRUE(frames[1].text());
    EXPECT_FALSE(frames[1].binary());
    std::vector<char> expected_data{ 'b', 'l', 'u', 'b' };
    EXPECT_EQ(frames[1].data, expected_data);
  }
}

TEST(websocket, sequence_binary_frame) {
  TestCtx ctx;
  TestSocket socket;
  TestChannel channel(ctx, socket);

  std::vector<websocket::frame> frames;
  auto task = frameSequence(channel, frames);
  task.start();
  EXPECT_FALSE(task.done());
  ctx.resume(0x02); // fin=0 opcode=2
  ctx.resume(0x84); // mask=1 len=4
  ctx.resume(0x00); // mask[0]
  ctx.resume(0x00); // mask[1]
  ctx.resume(0x00); // mask[2]
  ctx.resume(0x00); // mask[3]
  ctx.resume('t');
  ctx.resume('e');
  ctx.resume('s');
  ctx.resume('t');
  EXPECT_FALSE(task.done());
  ctx.resume(0x82); // fin=1 opcode=2
  ctx.resume(0x84); // mask=1 len=4
  ctx.resume(0x00); // mask[0]
  ctx.resume(0x00); // mask[1]
  ctx.resume(0x00); // mask[2]
  ctx.resume(0x00); // mask[3]
  ctx.resume('b');
  ctx.resume('l');
  ctx.resume('u');
  ctx.resume('b');
  socket.close();
  ctx.resume();
  EXPECT_TRUE(task.done());
  EXPECT_TRUE(task.result());
  ASSERT_EQ(frames.size(), 2ull);

  {
    EXPECT_FALSE(frames[0].fin);
    EXPECT_FALSE(frames[0].text());
    EXPECT_TRUE(frames[0].binary());
    std::vector<char> expected_data{ 't', 'e', 's', 't' };
    EXPECT_EQ(frames[0].data, expected_data);
  }
  {
    EXPECT_TRUE(frames[1].fin);
    EXPECT_FALSE(frames[1].text());
    EXPECT_TRUE(frames[1].binary());
    std::vector<char> expected_data{ 'b', 'l', 'u', 'b' };
    EXPECT_EQ(frames[1].data, expected_data);
  }
}

TEST(websocket, sequence_ping_pong_frame) {
  TestCtx ctx;
  TestSocket socket;
  TestChannel channel(ctx, socket);

  std::vector<websocket::frame> frames;
  auto task = frameSequence(channel, frames);
  task.start();
  EXPECT_FALSE(task.done());
  ctx.resume(0x89); // fin=1 opcode=9
  ctx.resume(0x80); // mask=1 len=0
  ctx.resume(0x00); // mask[0]
  ctx.resume(0x00); // mask[1]
  ctx.resume(0x00); // mask[2]
  ctx.resume(0x00); // mask[3]
  EXPECT_EQ(socket._written.size(), 0ull);
  ctx.resume();
  ctx.resume();
  ASSERT_EQ(socket._written.size(), 2ull);
  EXPECT_EQ(socket._written[0], (char)0x8A);
  EXPECT_EQ(socket._written[1], (char)0x00);
  socket.close();
  ctx.resume();
  EXPECT_TRUE(task.done());
  EXPECT_TRUE(task.result());
  ASSERT_EQ(frames.size(), 0ull);
}

TEST(websocket, sequence_ping_pong_echo_frame) {
  TestCtx ctx;
  TestSocket socket;
  TestChannel channel(ctx, socket);

  std::vector<websocket::frame> frames;
  auto task = frameSequence(channel, frames);
  task.start();
  EXPECT_FALSE(task.done());
  ctx.resume(0x89); // fin=1 opcode=9
  ctx.resume(0x88); // mask=1 len=8
  ctx.resume(0x10); // mask[0]
  ctx.resume(0x02); // mask[1]
  ctx.resume(0x30); // mask[2]
  ctx.resume(0x04); // mask[3]
  ctx.resume(0x11); // data[0]
  ctx.resume(0x12); // data[1]
  ctx.resume(0x31); // data[2]
  ctx.resume(0x14); // data[3]
  ctx.resume(0x12); // data[4]
  ctx.resume(0x22); // data[5]
  ctx.resume(0x32); // data[6]
  ctx.resume(0x24); // data[7]
  EXPECT_EQ(socket._written.size(), 0ull);
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
  ASSERT_EQ(socket._written.size(), 10ull);
  EXPECT_EQ(socket._written[0], (char)0x8A);
  EXPECT_EQ(socket._written[1], (char)0x08);
  EXPECT_EQ(socket._written[2], (char)0x01);
  EXPECT_EQ(socket._written[3], (char)0x10);
  EXPECT_EQ(socket._written[4], (char)0x01);
  EXPECT_EQ(socket._written[5], (char)0x10);
  EXPECT_EQ(socket._written[6], (char)0x02);
  EXPECT_EQ(socket._written[7], (char)0x20);
  EXPECT_EQ(socket._written[8], (char)0x02);
  EXPECT_EQ(socket._written[9], (char)0x20);
  socket.close();
  ctx.resume();
  EXPECT_TRUE(task.done());
  EXPECT_TRUE(task.result());
  ASSERT_EQ(frames.size(), 0ull);
}

TEST(websocket, sequence_close_handshake) {
  TestCtx ctx;
  TestSocket socket;
  TestChannel channel(ctx, socket);

  std::vector<websocket::frame> frames;
  auto task = frameSequence(channel, frames);
  task.start();
  EXPECT_FALSE(task.done());
  ctx.resume(0x88); // fin=1 opcode=8
  ctx.resume(0x80); // mask=1 len=4
  ctx.resume(0x00); // mask[0]
  ctx.resume(0x00); // mask[1]
  ctx.resume(0x00); // mask[2]
  ctx.resume(0x00); // mask[3]
  EXPECT_EQ(socket._written.size(), 0ull);
  ctx.resume();
  ctx.resume();
  ASSERT_EQ(socket._written.size(), 2ull);
  EXPECT_EQ(socket._written[0], (char)0x88);
  EXPECT_EQ(socket._written[1], (char)0x00);
  EXPECT_TRUE(task.done());
  ctx.resume();
  EXPECT_TRUE(task.done());
  EXPECT_TRUE(task.result());
  ASSERT_EQ(frames.size(), 0ull);
}
