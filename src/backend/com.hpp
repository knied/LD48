////////////////////////////////////////////////////////////////////////////////

#ifndef BACKEND_COM_HPP
#define BACKEND_COM_HPP

////////////////////////////////////////////////////////////////////////////////

#include "coro.hpp"
#include <iostream>

////////////////////////////////////////////////////////////////////////////////

namespace com {

template<typename async_ctx, typename async_io_if>
class channel final {
public:
  channel(async_ctx& ctx, async_io_if& io)
    : m_ctx(ctx), m_io(io) {}

  coro::async_generator<char>
  async_char_stream() {
    while (true) {
      char c;
      try {
        auto bytes = co_await m_io.async_read(m_ctx, &c, 1);
        if (bytes != 1) {
          co_return; // connection was closed
        }
      } catch (std::runtime_error const& err) {
        std::cout << err.what() << std::endl;
        co_return; // error
      }
      
      co_yield c;
    }
  }
  coro::task<bool>
  async_read(char* buffer, std::size_t count) {
    std::size_t complete = 0;
    while (complete < count) {
      auto bytes = co_await m_io.async_read(m_ctx, buffer + complete,
                                            count - complete);
      if (bytes == 0) {
        co_return false; // connection was closed
      }
      complete += bytes;
    }
    co_return true;
  }
  coro::task<bool>
  async_write(char const* buffer, std::size_t count) {
    std::size_t complete = 0;
    while (complete < count) {
      auto bytes = co_await m_io.async_write(m_ctx, buffer + complete,
                                             count - complete);
      if (bytes == 0) {
        co_return false; // connection was closed
      }
      complete += bytes;
    }
    co_return true;
  }

private:
  async_ctx& m_ctx;
  async_io_if& m_io;
};

} // com

////////////////////////////////////////////////////////////////////////////////

#endif // BACKEND_COM_HPP

////////////////////////////////////////////////////////////////////////////////
