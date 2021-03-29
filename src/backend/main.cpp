#include "utils.hpp"
#include "http.hpp"
#include "websocket.hpp"
#include "fs.hpp"
#include "Socket.hpp"

#include <vector>
#include <iostream>

auto createListeners(char const* host, char const* port, TLSConfig const& tlsConfig) {
  std::vector<std::unique_ptr<Socket>> listeners;

  {
    // First try to bind to IPv6
    AddressOptions options(IPv6, TCP, host, port);
    for (auto& info : options) {
      std::cout << "Try to listen on " << info << std::endl;
      try {
        listeners.push_back(std::make_unique<Socket>(info, 10, tlsConfig));
      } catch (std::runtime_error& ex) {
        std::cout << "  Error: " << ex.what() << std::endl;
      }
    }
  }

  {
    // In case of no dual-stack: Try to manyally bind to IPv4
    AddressOptions options(IPv4, TCP, host, port);
    for (auto& info : options) {
      std::cout << "Try to listen on " << info << std::endl;
      try {
        listeners.push_back(std::make_unique<Socket>(info, 10, tlsConfig));
      } catch (std::runtime_error& ex) {
        std::cout << "  Skipping: " << ex.what() << std::endl;
      }
    }
  }

  std::cout << "Listening on:" << std::endl;
  for (auto& listener : listeners) {
    std::cout << "  " << *listener << std::endl;
  }

  return listeners;
}

static bool
websocketHandshake(http::request::headers const& headers,
                   http::response& response) {
  auto key = headers.find("Sec-WebSocket-Key");
  auto version = headers.find("Sec-WebSocket-Version");
  if (key == headers.end() || version == headers.end()) {
    response.set_status_code(http::response::status_code::BAD_REQUEST);
    return false;
  }
  if (version->second != "13") {
    response.set_status_code(http::response::status_code::BAD_REQUEST);
    return false;
  }
  std::string magicString = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
  utils::SHA1 acceptHash = utils::sha1(key->second + magicString);
  auto acceptHash64 = utils::base64(acceptHash.data, 20);
  response.set_status_code(http::response::status_code::SWITCHING_PROTOCOLS);
  response.get_headers().insert(std::make_pair("Upgrade", "websocket"));
  response.get_headers().insert(std::make_pair("Connection", "Upgrade"));
  response.get_headers().insert(std::make_pair("Sec-WebSocket-Accept", acceptHash64));
  return true;
}

using tls_channel = com::channel<event::scheduler, Socket>;
static coro::sync_task<void>
httpServer(event::scheduler& s,
           Socket client,
           fs::cache const& files) {
  tls_channel channel(s, client);
  auto chars = channel.async_char_stream();
  bool upgradeToWebsocket = false;
  for co_await (auto request : http::request::stream(chars)) {
      //std::cout << ">>>>" << std::endl;
      //std::cout << request;
      http::response response;
      if (request.get_method() == http::request::method::GET) {
        auto& headers = request.get_headers();
        auto upgrade = headers.find("Upgrade");
        if (upgrade != headers.end()) {
          if (upgrade->second == "websocket") {
            upgradeToWebsocket = websocketHandshake(headers, response);
          } else {
            response.set_status_code(http::response::status_code::NOT_IMPLEMENTED);
          }
        } else {
          auto uri = request.get_uri();
          if (uri == "/") {
            uri = "/index.html";
          }
          auto content = files.find(uri);
          if (content) {
            response.set_status_code(http::response::status_code::OK);
            response.set_content(content);
          } else {
            response.set_status_code(http::response::status_code::NOT_FOUND);
          }
        }
      } else {
        response.set_status_code(http::response::status_code::NOT_IMPLEMENTED);
      }
      auto buffer = response.serialize();
      //std::cout << "====" << std::endl;
      std::size_t sent = 0;
      auto toSend = buffer.size();
      while (sent < toSend) {
        //std::cout << response;
        auto bytes = co_await client.async_write(s, buffer.data() + sent, toSend - sent);
        if (bytes == 0) {
          std::cout << "closed: " << client << std::endl;
          co_return;
        }
        sent += bytes;
      }
      //std::cout << "<<<<" << std::endl;
      if (upgradeToWebsocket) {
        break;
      }
    }
  if (upgradeToWebsocket) {
    std::cout << "upgrade: " << client << std::endl;
    for co_await (auto frame : websocket::stream(channel)) {
        std::string msg(frame.data.begin(), frame.data.end());
        //std::cout << "websocket: " << msg << std::endl;
        if (!co_await websocket::async_send_text(channel, "Hey there!")) {
          break;
        }
        co_await websocket::async_send_close(channel);
      }
  }
  std::cout << "closed: " << client << std::endl;
}

coro::sync_task<void>
acceptLoop(event::scheduler& s,
           Socket& listener,
           fs::cache const& files) {
  std::vector<coro::sync_task<void>> connections;
  while (true) {
    std::cout << "accepting..." << std::endl;
    auto client = co_await listener.async_accept(s);
    if (!client) {
      std::cout << "accept failed" << std::endl;
      continue;
    }
    std::cout << "accepted: " << client << std::endl;
    connections.erase(
      std::remove_if(connections.begin(), connections.end(),
                     [](auto const& c) {
                       if (c.done()) {
                         try {
                           c.result();
                         } catch (std::runtime_error const& e) {
                           std::cout << "exception: " << e.what() << std::endl;
                         }
                       }
                       return c.done();
                     }),
      connections.end());
    connections.push_back(httpServer(s, std::move(client), files));
    std::cout << "connections: " << connections.size() << std::endl;
    connections.back().start();
  }
}

int main(int argc, char const* argv[]) {
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
