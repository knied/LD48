#include "Socket.hpp"

#include <vector>
#include <iostream>
#include <ev.h>

auto createListeners(char const* host, char const* port) {
  std::vector<Socket> listeners;

  {
    // First try to bind to IPv6
    AddressOptions options(IPv6, TCP, host, port);
    for (auto& info : options) {
      std::cout << "Try to listen on " << info << std::endl;
      try {
        listeners.push_back(Socket(info, 10));
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
        listeners.push_back(Socket(info, 10));
      } catch (std::runtime_error& ex) {
        std::cout << "  Skipping: " << ex.what() << std::endl;
      }
    }
  }

  std::cout << "Listening on:" << std::endl;
  for (auto& listener : listeners) {
    std::cout << "  " << listener << std::endl;
  }
  
  return listeners;
}

int main(int argc, char const* argv[]) {
  for (int i = 0; i < argc; ++i) {
    std::cout << "argv[" << i << "] = \"" << argv[i] << "\"" << std::endl;
  }

  auto listeners = createListeners(nullptr, "25678");
  
  return 0;
}
