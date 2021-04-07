#include "utils.hpp"
#include "http.hpp"
#include "websocket.hpp"
#include "fs.hpp"
#include "net.hpp"

#include <vector>
#include <iostream>
#include <sstream>
#include <functional>

static std::string
dateAndTime() {
  // NOTE: Not thread safe!
  auto now = std::chrono::system_clock::now();
  auto time = std::chrono::system_clock::to_time_t(now);
  std::stringstream ss;
  ss << std::put_time(std::localtime(&time), "%Y-%m-%d %X");
  return ss.str();
}

enum class ConnectionStatus {
  Ok, OkClose, Error, Upgrade
};

static ConnectionStatus
generateHttpErrorResponse(http::response::status_code code,
                          fs::cache const& files,
                          http::response& response) {
  response.set_status_code(code);
  response.get_headers().insert(std::make_pair("Connection", "close"));
  std::string url = "/" + std::to_string((int)code) + ".html";
  auto error = files.find(url);
  if (error) {
    response.set_content(error);
  }
  return ConnectionStatus::Error;
}

static ConnectionStatus
generateWebsocketHandshake(http::request const& request,
                           fs::cache const& files,
                           http::response& response) {
  auto& headers = request.get_headers();
  auto key = headers.find("sec-websocket-key");
  auto version = headers.find("sec-websocket-version");
  if (key == headers.end() || version == headers.end()) {
    return generateHttpErrorResponse(
      http::response::status_code::BAD_REQUEST, files, response);
  }
  if (version->second != "13") {
    return generateHttpErrorResponse(
      http::response::status_code::BAD_REQUEST, files, response);
  }
  std::string magicString = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
  utils::SHA1 acceptHash = utils::sha1(key->second + magicString);
  auto acceptHash64 = utils::base64(acceptHash.data, 20);
  response.set_status_code(http::response::status_code::SWITCHING_PROTOCOLS);
  response.get_headers().insert(std::make_pair("Upgrade", "websocket"));
  response.get_headers().insert(std::make_pair("Connection", "Upgrade"));
  response.get_headers().insert(std::make_pair("Sec-WebSocket-Accept", acceptHash64));
  return ConnectionStatus::Ok;
}

static bool
wantsToClose(http::request const& request) {
  auto& headers = request.get_headers();
  auto h = headers.find("connection");
  return h != headers.end() && h->second == "close";
}

static bool
wantsToUpgrade(http::request const& request, std::string& protocol) {
  auto& headers = request.get_headers();
  auto h = headers.find("upgrade");
  if (h != headers.end()) {
    protocol = h->second;
    return true;
  }
  return false;
}

static bool
hasHostHeader(http::request const& request, std::string& host) {
  auto& headers = request.get_headers();
  auto h = headers.find("host");
  if (h != headers.end()) {
    host = h->second;
    return true;
  }
  return false;
}

static ConnectionStatus
generateResponse(http::request const& request,
                 fs::cache const& files,
                 http::response& response) {
  switch (request.get_method()) {
  case http::request::method::GET: {
    std::string host;
    if (!hasHostHeader(request, host)) {
      return generateHttpErrorResponse(
          http::response::status_code::BAD_REQUEST, files, response);
    }
    
    bool closeOnClientRequest = false;
    std::string upgradeTo;
    if (wantsToClose(request)) {
      closeOnClientRequest = true;
    } else if (wantsToUpgrade(request, upgradeTo)) {
      if (upgradeTo == "websocket") {
        return generateWebsocketHandshake(request, files, response);
      } else {
        return generateHttpErrorResponse(
          http::response::status_code::NOT_IMPLEMENTED, files, response);
      }
    }
    auto uri = request.get_uri();
    if (uri == "/") {
      uri = "/index.html";
    }
    auto content = files.find(uri);
    if (!content) {
      return generateHttpErrorResponse(
        http::response::status_code::NOT_FOUND, files, response);
    }
    response.set_status_code(http::response::status_code::OK);
    response.set_content(content);
    if (closeOnClientRequest) {
      response.get_headers().insert(std::make_pair("Connection", "close"));
      return ConnectionStatus::OkClose;
    }
    return ConnectionStatus::Ok;
  } break;
  default:
    return generateHttpErrorResponse(
      http::response::status_code::NOT_IMPLEMENTED, files, response);
  }
}

class scoped_logger final {
public:
  template<typename tocket_type>
  scoped_logger(tocket_type const& client,
                std::string const& context)
    : _context(context) {
    std::stringstream ss;
    ss << client;
    _name = ss.str();
    std::cout << dateAndTime() << " - " << _name
              << " - begin(" << _context << ")" << std::endl;
  }
  ~scoped_logger() {
    std::cout << dateAndTime() << " - " << _name
              << " - end(" << _context << ")" << std::endl;
  }
  scoped_logger(scoped_logger const&) = delete;
  scoped_logger(scoped_logger&&) = delete;
  scoped_logger& operator = (scoped_logger const&) = delete;
  scoped_logger& operator = (scoped_logger&&) = delete;
  void noteworthy(http::request const& request,
                  http::response const& response) const {
    std::cout << dateAndTime() << " - " << _name
              << " - noteworthy(" << _context << ")" << std::endl;
    std::cout << ">>>>" << std::endl;
    std::cout << request;
    std::cout << "====" << std::endl;
    std::cout << response;
    std::cout << "<<<<" << std::endl;
  }
  void fatal(std::string const& what) {
    std::cout << dateAndTime() << " - " << _name
              << " - fatal(" << _context << ")" << std::endl;
    std::cout << what << std::endl;
  }
  void note(std::string const& what) {
    std::cout << dateAndTime() << " - " << _name
              << " - note(" << _context << "): "
              << what << std::endl;
  }
private:
  std::string _name;
  std::string _context;
};

template<typename socket_type>
static coro::sync_task<void>
httpServer(event::scheduler& s,
           socket_type client,
           fs::cache const& files) {
  using channel_type = com::channel<event::scheduler, socket_type>;
  scoped_logger logger(client, "https");
  channel_type channel(s, client);
  auto chars = channel.async_char_stream();
  ConnectionStatus status = ConnectionStatus::Ok;
  try {
  for co_await (auto request : http::request::stream(chars)) {
      http::response response;
      status = generateResponse(request, files, response);
      if (status != ConnectionStatus::Ok) {
        logger.noteworthy(request, response);
      }

      if (!co_await http::response::async_write(channel, response)) {
        co_return;
      }
      if (status != ConnectionStatus::Ok) {
        break;
      }
    }
  if (status == ConnectionStatus::Upgrade) {
#if 0
    std::cout << dateAndTime() << " - upgrade: " << clientName << std::endl;
    for co_await (auto frame : websocket::stream(channel)) {
        std::string msg(frame.data.begin(), frame.data.end());
        if (!co_await websocket::async_send_text(channel, "Hey there!")) {
          break;
        }
        co_await websocket::async_send_close(channel);
      }
#else
    throw std::runtime_error("error: Ignoring attempt to upgrade");
#endif
  }
  } catch (std::runtime_error& err) {
    logger.fatal(err.what());
  }
}

using open_channel = com::channel<event::scheduler, net::socket>;
static coro::sync_task<void>
httpsForwarder(event::scheduler& s,
               net::socket client) {
  scoped_logger logger(client, "http");
  open_channel channel(s, client);
  auto chars = channel.async_char_stream();
  try {
    for co_await (auto request : http::request::stream(chars)) {
        std::string host;
        http::response response;
        if (!hasHostHeader(request, host)) {
          // Host header is required
          // TODO: Describe reason in message body?
          response.set_status_code(http::response::status_code::BAD_REQUEST);
          response.get_headers().insert(std::make_pair("Connection", "close"));
        } else {
          // TODO: be more strict about the host header?
          auto location = "https://" + host + request.get_uri();
          response.set_status_code(http::response::status_code::MOVED_PERMANENTLY);
          response.get_headers().insert(std::make_pair("Location", location));
          response.get_headers().insert(std::make_pair("Connection", "close"));
        }
    
        logger.noteworthy(request, response);

        if (!co_await http::response::async_write(channel, response)) {
          co_return;
        }
        co_return;
      }
  } catch (std::runtime_error& err) {
    logger.fatal(err.what());
  }
}

using open_channel = com::channel<event::scheduler, net::socket>;
static coro::sync_task<void>
controlHandler(event::scheduler& s,
               net::socket client) {
  bool shutdown = false;
  scoped_logger logger(client, "control");
  open_channel channel(s, client);
  auto chars = channel.async_char_stream();
  try {
    for co_await (auto request : http::request::stream(chars)) {
        std::string host;
        http::response response;
        if (request.get_method() != http::request::method::GET ||
            !hasHostHeader(request, host) ||
            request.get_uri() != "/shutdown") {
          // TODO: Describe reason in message body?
          response.set_status_code(http::response::status_code::BAD_REQUEST);
          response.get_headers().insert(std::make_pair("Connection", "close"));
        } else {
          response.set_status_code(http::response::status_code::OK);
          response.get_headers().insert(std::make_pair("Connection", "close"));
          shutdown = true;
        }
        logger.noteworthy(request, response);
        if (!co_await http::response::async_write(channel, response)) {
          co_return;
        }
        if (shutdown) {
          logger.note("shutdown");
          s.trigger_shutdown_from_task();
        }
        co_return;
      }
  } catch (std::runtime_error& err) {
    logger.fatal(err.what());
  }
}

template<typename socket_type, typename handler_type>
coro::sync_task<void>
acceptor(event::scheduler& s, socket_type listener,
         handler_type handler) {
  scoped_logger logger(listener, "listener");
  while (true) {
    try {
      auto client = co_await listener.async_accept(s);
      if (!client) {
        logger.fatal("accept failed");
        continue;
      }
      logger.note("accepted");
      //std::cout << dateAndTime() << " - " << client << " - accepted" << std::endl;
      s.execute(handler(std::move(client)));
    } catch (std::runtime_error& err) {
      logger.fatal(err.what());
    }
  }
}

template<typename socket_type, typename... arg_types>
auto createListeners(char const* host, char const* port, arg_types&& ...args) {
  std::vector<socket_type> listeners;
  {
    // First try to bind to IPv6
    net::address_options options(net::IPv6, net::TCP, host, port);
    for (auto& info : options) {
      std::cout << "Try to listen on " << info << std::endl;
      try {
        auto l = socket_type(info, 10, args...);
        std::cout << "  Listening: " << l << std::endl;
        listeners.push_back(std::move(l));
        
      } catch (std::runtime_error& ex) {
        std::cout << "  Skipping: " << ex.what() << std::endl;
      }
    }
  }
  {
    // In case of no dual-stack: Try to manually bind to IPv4
    net::address_options options(net::IPv4, net::TCP, host, port);
    for (auto& info : options) {
      std::cout << "Try to listen on " << info << std::endl;
      try {
        auto l = socket_type(info, 10, args...);
        std::cout << "  Listening: " << l << std::endl;
        listeners.push_back(std::move(l));
      } catch (std::runtime_error& ex) {
        std::cout << "  Skipping: " << ex.what() << std::endl;
      }
    }
  }
  return listeners;
}

int main(int argc, char const* argv[]) {
  std::cout << dateAndTime() << " - Launching Server" << std::endl;
  for (int i = 0; i < argc; ++i) {
    std::cout << "argv[" << i << "] = \"" << argv[i] << "\"" << std::endl;
  }
  bool devMode = false;
  std::string path(".");
  std::string cert;
  std::string key;
  for (int i = 0; i < argc; ++i) {
    if (std::string(argv[i]) == "--root") {
      assert(i+1 < argc);
      path = std::string(argv[++i]);
    }
    if (std::string(argv[i]) == "--cert") {
      assert(i+1 < argc);
      cert = std::string(argv[++i]);
    }
    if (std::string(argv[i]) == "--key") {
      assert(i+1 < argc);
      key = std::string(argv[++i]);
    }
    if (std::string(argv[i]) == "--dev") {
      devMode = true;
    }
  }

  event::scheduler s;
  std::vector<coro::sync_task<void>> tasks;
  fs::cache files(path, devMode);
  auto controlListeners = createListeners<net::socket>(nullptr, "6789");
  if (devMode) {
    auto httpListeners = createListeners<net::socket>(nullptr, "8080");
    for (auto& listener : httpListeners) {
      s.execute(acceptor(s, std::move(listener), [&s, &files](auto client) {
        return httpServer(s, std::move(client), files);
      }));
    }
    for (auto& listener : controlListeners) {
      s.execute(acceptor(s, std::move(listener), [&s](auto client) {
        return controlHandler(s, std::move(client));
      }));
    }
  } else {
    crypto::config tlsConfig(cert, key);
    auto httpsListeners = createListeners<net::tls_socket>(nullptr, "443", tlsConfig);
    auto httpListeners = createListeners<net::socket>(nullptr, "80");
    for (auto& listener : httpsListeners) {
      s.execute(acceptor(s, std::move(listener), [&s, &files](auto client) {
        return httpServer(s, std::move(client), files);
      }));
    }
    for (auto& listener : httpListeners) {
      s.execute(acceptor(s, std::move(listener), [&s](auto client) {
        return httpsForwarder(s, std::move(client));
      }));
    }
    for (auto& listener : controlListeners) {
      s.execute(acceptor(s, std::move(listener), [&s](auto client) {
        return controlHandler(s, std::move(client));
      }));
    }
  }
  return s.run();
}
