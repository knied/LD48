////////////////////////////////////////////////////////////////////////////////

#ifndef BACKEND_WEBSOCKET_HPP
#define BACKEND_WEBSOCKET_HPP

////////////////////////////////////////////////////////////////////////////////

#include "utils.hpp"
#include "com.hpp"
#include <vector>
#include <iostream> // to be removed

////////////////////////////////////////////////////////////////////////////////

namespace websocket {

////////////////////////////////////////////////////////////////////////////////
//       0               1               2               3
//       7 6 5 4 3 2 1 0 7 6 5 4 3 2 1 0 7 6 5 4 3 2 1 0 7 6 5 4 3 2 1 0
//      +-+-+-+-+-------+-+-------------+-------------------------------+
//      |F|R|R|R| opcode|M| Payload len |    Extended payload length    |
//      |I|S|S|S|  (4)  |A|     (7)     |             (16/64)           |
//      |N|V|V|V|       |S|             |   (if payload len==126/127)   |
//      | |1|2|3|       |K|             |                               |
//      +-+-+-+-+-------+-+-------------+ - - - - - - - - - - - - - - - +
//      |     Extended payload length continued, if payload len == 127  |
//      + - - - - - - - - - - - - - - - +-------------------------------+
//      |                               | Masking-key, if MASK set to 1 |
//      +-------------------------------+-------------------------------+
//      | Masking-key (continued)       |           Payload Data        |
//      +-------------------------------- - - - - - - - - - - - - - - - +
//      :                     Payload Data continued ...                :
//      + - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - +
//      |                     Payload Data continued ...                |
//      +---------------------------------------------------------------+
////////////////////////////////////////////////////////////////////////////////

enum class opcode : std::uint8_t {
  CONTINUATION_FRAME = 0,
  TEXT_FRAME = 1,
  BINARY_FRAME = 2,
  CLOSE = 8,
  PING = 9,
  PONG = 10
};

static constexpr inline bool
is_control(opcode code) {
  return (std::uint8_t)code >= 8;
}

struct frame final {
  frame() = default;
  frame(frame&& other)
    : fin(other.fin)
    , code(other.code)
    , data(std::move(other.data)) {}
  frame& operator = (frame&& other) {
    fin = other.fin;
    code = other.code;
    data = std::move(other.data);
    return *this;
  }
  frame(frame const&) = delete;
  frame& operator = (frame const&) = delete;
  ~frame() = default;

  bool text() const { return code == opcode::TEXT_FRAME; }
  bool binary() const { return code == opcode::BINARY_FRAME; }

  bool fin = true;
  opcode code = opcode::CONTINUATION_FRAME;
  std::vector<char> data;
}; // struct fram

template<typename async_ctx, typename async_io_if>
static coro::task<bool>
async_read(com::channel<async_ctx, async_io_if>& channel, frame& out) {
  char buffer[8];
  if (!co_await channel.async_read(buffer, 2)) {
    // unexpected eof
    co_return false;
  }
  bool fin = (buffer[0] & 0x80) == 0x80;
  opcode code = (opcode)(buffer[0] & 0x0F);
  bool mask = (buffer[1] & 0x80) == 0x80;
  std::uint64_t length = (std::uint64_t)(buffer[1] & 0x7F);
  if (is_control(code) && length > 125) {
    // illegal
    co_return false;
  }
  if (is_control(code) && fin == false) {
    // illegal
    co_return false;
  }
  if (length == 126) {
    if (!co_await channel.async_read(buffer, 2)) {
      // unexpected eof
      co_return false;
    }
    length = utils::read_be<std::uint16_t>(buffer);
  } else if (length == 127) {
    if (!co_await channel.async_read(buffer, 8)) {
      // unexpected eof
      co_return false;
    }
    length = utils::read_be<std::uint64_t>(buffer);
  }
  if (!mask) {
    // illegal
    std::cout << "error: mask required!" << std::endl;
    co_return false;
  }
  if (!co_await channel.async_read(buffer, 4)) {
    // unexpected eof
    std::cout << "error: unexpected eof!" << std::endl;
    co_return false;
  }

  std::vector<char> data(length);
  if (!co_await channel.async_read(data.data(), length)) {
    // unexpected eof
    co_return false;
  }
  for (std::size_t i = 0; i < length; ++i) {
    data[i] ^= buffer[i % 4];
  }
  out.fin = fin;
  out.code = code;
  out.data = std::move(data);
  co_return true;
}

template<typename async_ctx, typename async_io_if>
static coro::task<bool>
async_write(com::channel<async_ctx, async_io_if>& channel, frame const& in) {
  char buffer[10];
  buffer[0] = (char)(in.fin ? 0x80 : 0x00) | (char)in.code;
  auto count = in.data.size();
  std::size_t headerSize = 0;
  if (count > 65535) {
    buffer[1] = 127;
    utils::write_be(buffer + 2, (std::uint64_t)count);
    headerSize = 10;
  } else if (count > 125) {
    buffer[1] = 126;
    utils::write_be(buffer + 2, (std::uint16_t)count);
    headerSize = 4;
  } else {
    buffer[1] = (char)count;
    headerSize = 2;
  }
  if (!co_await channel.async_write(buffer, headerSize)) {
    // unexpected eof
    co_return false;
  }
  if (!co_await channel.async_write(in.data.data(), in.data.size())) {
    // unexpected eof
    co_return false;
  }
  co_return true;
}

template<typename async_ctx, typename async_io_if>
static coro::task<bool>
async_send_text(com::channel<async_ctx, async_io_if>& channel,
                std::string const& text) {
  frame f;
  f.fin = true;
  f.code = opcode::BINARY_FRAME;
  f.data = std::vector<char>(text.begin(), text.end());
  if (!co_await async_write(channel, f)) {
    co_return false;
  }
  co_return true;
}

template<typename async_ctx, typename async_io_if>
static coro::task<bool>
async_send_close(com::channel<async_ctx, async_io_if>& channel) {
  frame f;
  f.fin = true;
  f.code = opcode::CLOSE;
  if (!co_await async_write(channel, f)) {
    co_return false;
  }
  co_return true;
}

template<typename async_ctx, typename async_io_if>
static coro::async_generator<frame>
stream(com::channel<async_ctx, async_io_if>& channel) {
  bool continuation_fin = true;
  opcode continuation_code = opcode::CONTINUATION_FRAME;
  while (true) {
    frame f;
    if (!co_await async_read(channel, f)) {
      co_return;
    }
    if (f.code == opcode::CONTINUATION_FRAME) {
      if (continuation_fin == true ||
          continuation_code == opcode::CONTINUATION_FRAME) {
        std::cout << "error: spurious continuation!" << std::endl;
        co_return;
      }
      continuation_fin = f.fin;
      f.code = continuation_code;
    } else {
      continuation_fin = f.fin;
      continuation_code = f.code;
    }
    if (f.code == opcode::CLOSE) {
      if (!co_await async_write(channel, f)) {
        co_return;
      }
      break;
    } else if (f.code == opcode::PING) {
      f.code = opcode::PONG;
      if (!co_await async_write(channel, f)) {
        co_return;
      }
    } else if (f.code == opcode::PONG) {
      // ignore
    } else {
      co_yield std::move(f);
    }
  }
}

} // namespace websocket

////////////////////////////////////////////////////////////////////////////////

#endif // BACKEND_WEBSOCKET_HPP

////////////////////////////////////////////////////////////////////////////////
