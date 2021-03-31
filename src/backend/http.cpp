////////////////////////////////////////////////////////////////////////////////

#include "http.hpp"

////////////////////////////////////////////////////////////////////////////////

namespace http {

static constexpr char const*
methodToString(request::method m) {
  switch (m) {
  case request::method::GET: return "GET";
  }
}

static constexpr char const*
reasonPhrase(response::status_code sc) {
  switch (sc) {
  case response::status_code::SWITCHING_PROTOCOLS: return "Switching Protocols";
  case response::status_code::OK: return "OK";
  case response::status_code::BAD_REQUEST: return "Bad Request";
  case response::status_code::NOT_FOUND: return "Not Found";
  case response::status_code::NOT_IMPLEMENTED: return "Not Implemented";
  }
}

static constexpr char const*
mimeTypeToString(content::mime_type t) {
  switch (t) {
  case content::mime_type::HTML: return "text/html";
  case content::mime_type::JS: return "text/javascript";
  case content::mime_type::CSS: return "text/css";
  case content::mime_type::WASM: return "application/wasm";
  case content::mime_type::TEXT: return "text/plain";
  }
}

static constexpr bool
isWhitespace(char c) {
  return c == ' ' || c == '\t';
}

static coro::async_generator<std::string>
messageLines(coro::async_generator<char>& chars) {
  // TODO(security): Is there a maximum line length? Should I define one?
  std::string line;
  for co_await (auto c : chars) {
    if (c != '\r' && c != '\n') {
      line += c;
    } else if (c == '\n') {
      std::string tmp;
      std::swap(tmp, line);
      co_yield std::move(tmp);
    }
  }
}

class string_pointer {
public:
  using const_iterator = char const*;
  explicit string_pointer(char const* pointer, std::size_t length)
    : mPointer(pointer)
    , mLength(length) {}
  explicit string_pointer(const_iterator a, const_iterator b)
    : mPointer(a)
    , mLength(b - a) {}
  const_iterator begin() const { return mPointer; }
  const_iterator end() const { return mPointer + mLength; }

  std::string str() const {
    return std::string(mPointer, mLength);
  }

  bool operator == (char const* str) const {
    std::size_t l = 0;
    while (l < mLength) {
      auto co = str[l];
      auto c = mPointer[l++];
      if (co == '\0' || co != c) {
        return false;
      }
    }
    return str[l] == '\0';
  }
  bool operator != (char const* str) const {
    return !(*this == str);
  }

private:
  char const* mPointer;
  std::size_t mLength;
};

static std::ostream&
operator << (std::ostream& stream, string_pointer const& str) {
  stream << std::string(str.begin(), str.end());
  return stream;
}

class token : public string_pointer {
public:
  token(string_pointer const& str, char separator = 0)
    : string_pointer(str)
    , mSeparator(separator) {}
  char separator() const { return mSeparator; }
private:
  char mSeparator;
};

static coro::generator<token>
tokenize(string_pointer const& str, std::vector<char> const& separators) {
  auto it0 = str.begin();
  auto it1 = it0;
  auto end = str.end();
  while (it1 != end) {
    auto c = *it1;
    for (auto s : separators) {
      if (c == s) {
        co_yield token(string_pointer(it0, it1), s);
        it0 = it1+1;
        break;
      }
    }
    it1++;
  }
  if (it0 != end) {
    co_yield token(string_pointer(it0, it1));
  }
}

static bool
parseRequestLine(std::string const& line, request& req) {
  std::vector<char> separators{' '};
  string_pointer str(line.c_str(), line.size());
  auto tokens = tokenize(str, separators);
  auto it = tokens.begin();
  auto end = tokens.end();
  if (it == end) {
    std::cout << "Missing request method token" << std::endl;
    return false;
  }
  auto methodToken = *it;
  if (++it == end) {
    std::cout << "Missing uri token" << std::endl;
    return false;
  }
  auto uriToken = *it;
  if (++it == end) {
    std::cout << "Missing version token" << std::endl;
    return false;
  }
  auto versionToken = *it;
  if (++it != end) {
    std::cout << "Unexpected additional tokens" << std::endl;
    return false;
  }
  if (versionToken != "HTTP/1.1") {
    std::cout << "Unsupported HTTP version " << versionToken << std::endl;
    return false;
  }
  if (methodToken != "GET") {
    std::cout << "Unsupported request method " << methodToken << std::endl;
    return false;
  }
  req.set_method(request::method::GET);
  req.set_uri(uriToken.str());
  return true;
}

static bool
parseMessageHeader(std::string const& line, request& req) {
  auto p0 = line.find(':');
  if (p0 == std::string::npos) {
    std::cout << "No ':' separator found" << std::endl;
    return false;
  }
  auto p1 = p0+1;
  while (p1 < line.size() && isWhitespace(line[p1])) {
    p1++;
  }
  req.get_headers().insert(std::make_pair(line.substr(0, p0), line.substr(p1)));
  return true;
}

coro::async_generator<request>
request::stream(coro::async_generator<char>& chars) {
  auto lines = messageLines(chars);
  auto it = co_await lines.begin();
  auto end = lines.end();
  while(it != end) {
    auto request_line = *it;
    //co_await ++it;
    //std::cout << "request_line: " << request_line << std::endl;
    request req;
    if (!parseRequestLine(request_line, req)) {
      std::cout << "Malformed request-line:" << std::endl;
      std::cout << request_line << std::endl;
      co_return;
    }

    std::string multiLine;
    while ((co_await ++it) != end) {
      auto line = *it;
      //std::cout << "line: " << line << std::endl;
      if ((line.size() == 0 || !isWhitespace(line[0])) && multiLine.size()) {
        if (!parseMessageHeader(multiLine, req)) {
          std::cout << "Malformed message-header" << std::endl;
          std::cout << multiLine << std::endl;
          co_return;
        }
        multiLine.clear();
      }
      multiLine += line;
      if (line.size() == 0) {
        co_yield std::move(req);
        co_await ++it;
        break;
      }
    }
  }
}

std::ostream&
operator << (std::ostream& stream, request const& r) {
  stream << methodToString(r.get_method()) << " " << r.get_uri() << " HTTP/1.1\n";
  for (auto h : r.get_headers()) {
    stream << h.first << ": " << h.second << std::endl;
  }
  return stream;
}

std::vector<char>
response::serialize() const {
  std::vector<char> result;
  std::string header = "HTTP/1.1 " + std::to_string((int)mStatusCode) + " " + reasonPhrase(mStatusCode) + "\r\n";
  if (mContent) {
    header += "Content-Length: " + std::to_string(mContent->size()) + "\r\n";
    header += "Content-Location: " + mContent->location() + "\r\n";
    header += "Content-Type: " + std::string(mimeTypeToString(mContent->type())) + "\r\n";
  }
  for (auto h : mHeaders) {
    header += h.first + ": " + h.second + "\r\n";
  }
  header += "\r\n";
  result.insert(result.end(), header.begin(), header.end());
  if (mContent) {
    result.insert(result.end(), mContent->data(), mContent->data() + mContent->size());
  }
  return result;
}

std::ostream&
operator << (std::ostream& stream, response const& r) {
  auto sc = r.get_status_code();
  stream << "HTTP/1.1 " << (int)sc << " " << reasonPhrase(sc) << "\n";
  auto content = r.get_content();
  if (content) {
    stream << "Content-Length: " << content->size() << std::endl;
    stream << "Content-Location: " << content->location() << std::endl;
    stream << "Content-Type: " << mimeTypeToString(content->type()) << std::endl;
  }
  for (auto h : r.get_headers()) {
    stream << h.first << ": " << h.second << std::endl;;
  }
  return stream;
}

} // namespance http

////////////////////////////////////////////////////////////////////////////////
