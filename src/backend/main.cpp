#include "utils.hpp"
#include "http.hpp"
#include "websocket.hpp"
#include "fs.hpp"
#include "Socket.hpp"

#include <vector>
#include <iostream>
#include <sstream>

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
  auto error = files.find(url, true);
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
  auto key = headers.find("Sec-WebSocket-Key");
  auto version = headers.find("Sec-WebSocket-Version");
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
  auto h = headers.find("Connection");
  return h != headers.end() && h->second == "close";
}

static bool
wantsToUpgrade(http::request const& request, std::string& protocol) {
  auto& headers = request.get_headers();
  auto h = headers.find("Upgrade");
  if (h != headers.end()) {
    protocol = h->second;
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
    auto content = files.find(uri, true);
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

using tls_channel = com::channel<event::scheduler, Socket>;
static coro::sync_task<void>
httpServer(event::scheduler& s,
           Socket client,
           fs::cache const& files) {
  tls_channel channel(s, client);
  auto chars = channel.async_char_stream();
  ConnectionStatus status = ConnectionStatus::Ok;
  for co_await (auto request : http::request::stream(chars)) {
      http::response response;
      status = generateResponse(request, files, response);
      if (status != ConnectionStatus::Ok) {
        std::cout << dateAndTime() << " - Noteworthy HTTP Request:" << std::endl;
        std::cout << ">>>>" << std::endl;
        std::cout << request;
        std::cout << "====" << std::endl;
        std::cout << response;
        std::cout << "<<<<" << std::endl;
      }
      
      auto buffer = response.serialize();
      std::size_t sent = 0;
      auto toSend = buffer.size();
      while (sent < toSend) {
        auto bytes = co_await client.async_write(s, buffer.data() + sent, toSend - sent);
        if (bytes == 0) {
          std::cout << dateAndTime() << " - closed: " << client << std::endl;
          co_return;
        }
        sent += bytes;
      }
      if (status != ConnectionStatus::Ok) {
        break;
      }
    }
  if (status == ConnectionStatus::Upgrade) {
    std::cout << dateAndTime() << " - upgrade: " << client << std::endl;
    for co_await (auto frame : websocket::stream(channel)) {
        std::string msg(frame.data.begin(), frame.data.end());
        if (!co_await websocket::async_send_text(channel, "Hey there!")) {
          break;
        }
        co_await websocket::async_send_close(channel);
      }
  }
  std::cout << dateAndTime() << " - closed: " << client << std::endl;
}

coro::sync_task<void>
acceptLoop(event::scheduler& s,
           Socket& listener,
           fs::cache const& files) {
  std::vector<coro::sync_task<void>> connections;
  while (true) {
    //std::cout << "accepting..." << std::endl;
    auto client = co_await listener.async_accept(s);
    if (!client) {
      std::cout << dateAndTime() << " - accept failed" << std::endl;
      continue;
    }
    std::size_t garbage = 0;
    connections.erase(
      std::remove_if(connections.begin(), connections.end(),
                     [&garbage](auto const& c) {
                       if (c.done()) {
                         garbage++;
                         try {
                           c.result();
                         } catch (std::runtime_error const& e) {
                           std::cout << "exception: " << e.what() << std::endl;
                         }
                       }
                       return c.done();
                     }),
      connections.end());
    std::stringstream ss;
    ss << client;
    connections.push_back(httpServer(s, std::move(client), files));
    std::cout << dateAndTime() << " - accepted: " << ss.str() << " #connections = " << connections.size() << " #garbage = " << garbage << std::endl;
    connections.back().start();
  }
}

auto createListeners(char const* host, char const* port, TLSConfig const& tlsConfig) {
  std::vector<std::unique_ptr<Socket>> listeners;

  {
    // First try to bind to IPv6
    AddressOptions options(IPv6, TCP, host, port);
    for (auto& info : options) {
      std::cout << "Try to listen on " << info << std::endl;
      try {
        auto l = std::make_unique<Socket>(info, 10, tlsConfig);
        std::cout << "  Listening: " << *l << std::endl;
        listeners.push_back(std::move(l));
        
      } catch (std::runtime_error& ex) {
        std::cout << "  Skipping: " << ex.what() << std::endl;
      }
    }
  }

  {
    // In case of no dual-stack: Try to manyally bind to IPv4
    AddressOptions options(IPv4, TCP, host, port);
    for (auto& info : options) {
      std::cout << "Try to listen on " << info << std::endl;
      try {
        auto l = std::make_unique<Socket>(info, 10, tlsConfig);
        std::cout << "  Listening: " << *l << std::endl;
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
  }

  TLSConfig tlsConfig(cert, key);

  event::scheduler s;
  auto listeners = createListeners(nullptr, "443", tlsConfig);

  fs::cache files(path);

  std::vector<coro::sync_task<void>> tasks;
  for (auto& listener : listeners) {
    tasks.push_back(acceptLoop(s, *listener, files));
    tasks.back().start();
  }
  s.run();
  
  return 0;
}
