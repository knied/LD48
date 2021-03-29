// GET / HTTP/1.1
// Host: localhost
// Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8
// Accept-Language: en-gb
// Connection: keep-alive
// Accept-Encoding: gzip, deflate, br
// User-Agent: Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_4) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/13.1 Safari/605.1.15
////////////////////////////////////////////////////////////////////////////////

#include "../http.hpp"
#include <gtest/gtest.h>

////////////////////////////////////////////////////////////////////////////////

static coro::async_generator<char>
char_feeder(std::string str) {
  for (auto c : str) {
    co_yield c;
  }
}

template<typename T>
static coro::generator<T>
blocking(coro::async_generator<T> g) {
  for (auto it = g.begin(); it != g.end(); ++it) {
    auto t = [&it]() -> coro::sync_task<void> {
      co_await it;
    }();
    t.start();
    while (!t.done()) {
      assert(false && "should not block");
    }
    if (it != g.end()) {
      co_yield *it;
    }
  }
}

TEST(http, request_stream) {
  auto s = "GET / HTTP/1.1\r\n"
    "Host: localhost\r\n"
    "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n"
    "Accept-Language: en-gb\r\n"
    "Connection: keep-alive\r\n"
    "Accept-Encoding: gzip, deflate, br\r\n"
    "User-Agent: Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_4) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/13.1 Safari/605.1.15\r\n"
    "\r\n"
    "GET /hello/world.txt HTTP/1.1\r\n"
    "Key: value\r\n"
    "\r\n"
    "GET /some/uri HTTP/1.1\r\n"
    "Header0: split\r\n"
    " over\r\n"
    "\tmultiple\r\n"
    " \tlines\r\n"
    "Regular: header\r\n"
    "\r\n"
    "GET bla HTTP/1.1\r\n";

  std::vector<http::request> expected;
  expected.resize(3);
  
  //expected[0].addLine("GET / HTTP/1.1");
  expected[0].set_method(http::request::method::GET);
  expected[0].set_uri("/");
  expected[0].get_headers().insert(std::make_pair("Host", "localhost"));
  expected[0].get_headers().insert(std::make_pair("Accept", "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8"));
  expected[0].get_headers().insert(std::make_pair("Accept-Language", "en-gb"));
  expected[0].get_headers().insert(std::make_pair("Connection", "keep-alive"));
  expected[0].get_headers().insert(std::make_pair("Accept-Encoding", "gzip, deflate, br"));
  expected[0].get_headers().insert(std::make_pair("User-Agent", "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_4) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/13.1 Safari/605.1.15"));

  //expected[1].addLine("GET / HTTP/1.1");
  expected[1].set_method(http::request::method::GET);
  expected[1].set_uri("/hello/world.txt");
  expected[1].get_headers().insert(std::make_pair("Key", "value"));

  expected[2].set_method(http::request::method::GET);
  expected[2].set_uri("/some/uri");
  expected[2].get_headers().insert(std::make_pair("Header0", "split over\tmultiple \tlines"));
  expected[2].get_headers().insert(std::make_pair("Regular", "header"));

  auto it = expected.begin();

  auto chars = char_feeder(s);
  for (auto& r : blocking(http::request::stream(chars))) {
    ASSERT_TRUE(it != expected.end());
    auto& e = *(it++);
    EXPECT_EQ(e.get_method(), r.get_method());
    EXPECT_EQ(e.get_uri(), r.get_uri());
    EXPECT_EQ(e.get_headers(), r.get_headers());
  }
  EXPECT_TRUE(it == expected.end());
}

////////////////////////////////////////////////////////////////////////////////
