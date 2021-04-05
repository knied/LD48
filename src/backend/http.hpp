////////////////////////////////////////////////////////////////////////////////

#ifndef BACKEND_HTTP_HPP
#define BACKEND_HTTP_HPP

////////////////////////////////////////////////////////////////////////////////

#include <common/coro.hpp>
#include <string>
#include <vector>
#include <map>
#include <iostream>

////////////////////////////////////////////////////////////////////////////////

namespace http {

class content {
public:
  enum class mime_type {
    HTML, JS, CSS, WASM, TEXT
  };
  virtual std::size_t size() const = 0;
  virtual char const* data() const = 0;
  virtual std::string const& location() const = 0;
  virtual mime_type type() const = 0;
};

class request final {
public:
  enum class method {
    GET
  };
  using headers = std::map<std::string, std::string>;
  
  request() = default;
  ~request() = default;
  request(request&& other)
    : mMethod(std::move(other.mMethod))
    , mUri(std::move(other.mUri))
    , mHeaders(std::move(other.mHeaders)) {}
  request(request const&) = delete;
  request& operator = (request&& other) {
    if (&other != this) {
      std::swap(mMethod, other.mMethod);
      std::swap(mUri, other.mUri);
      std::swap(mHeaders, other.mHeaders);
    }
    return *this;
  }
  request& operator = (request const&) = delete;

  void set_method(method m) { mMethod = m; }
  method get_method() const { return mMethod; }

  void set_uri(std::string const& uri) { mUri = uri; }
  std::string const& get_uri() const { return mUri; }

  headers& get_headers() {
    return mHeaders;
  }
  headers const& get_headers() const {
    return mHeaders;
  }

  // TODO: I dont like this API anymore
  // * Should use com::channel
  // * Reading a single request is hacky
  static coro::async_generator<http::request>
  stream(coro::async_generator<char>& chars);
  
private:
  method mMethod = method::GET;
  std::string mUri;
  headers mHeaders;
};

// NOTE: Only for printing!
std::ostream&
operator << (std::ostream& stream, request const& r);

class response final {
public:
  enum class status_code : int {
    SWITCHING_PROTOCOLS=101,
    OK=200,
    MOVED_PERMANENTLY=301,
    BAD_REQUEST=400,
    NOT_FOUND=404,
    NOT_IMPLEMENTED=501
  };
  using headers = std::map<std::string, std::string>;
  
  response() = default;
  ~response() = default;
  response(response&& other)
    : mStatusCode(std::move(other.mStatusCode))
    , mContent(std::move(other.mContent))
    , mHeaders(std::move(other.mHeaders)) {}
  response(response const&) = delete;
  response& operator = (response&& other) {
    if (&other != this) {
      std::swap(mStatusCode, other.mStatusCode);
      std::swap(mContent, other.mContent);
      std::swap(mHeaders, other.mHeaders);
    }
    return *this;
  }
  response& operator = (response const&) = delete;

  void set_status_code(status_code sc) {
    mStatusCode = sc;
  }
  status_code get_status_code() const {
    return mStatusCode;
  }
  headers& get_headers() {
    return mHeaders;
  }
  headers const& get_headers() const {
    return mHeaders;
  }

  void set_content(content const* c) {
    mContent = c;
  }
  content const* get_content() const {
    return mContent;
  }

  std::vector<char> serialize() const;

  template<typename channel>
  static coro::task<bool>
  async_write(channel& c, response const& r) {
    auto buffer = r.serialize();
    co_return co_await c.async_write(buffer.data(), buffer.size());
  }

private:
  status_code mStatusCode = status_code::OK;
  content const* mContent = nullptr;
  headers mHeaders;
};

// NOTE: Only for printing!
std::ostream& operator << (std::ostream& stream, response const& r);

} // namespace http

////////////////////////////////////////////////////////////////////////////////

#endif // BACKEND_HTTP_HPP

////////////////////////////////////////////////////////////////////////////////
