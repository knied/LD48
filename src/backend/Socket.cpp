////////////////////////////////////////////////////////////////////////////////

#include "Socket.hpp"

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h>
#include <errno.h>
#include <iostream>
#include <cassert>
#include <string.h>

////////////////////////////////////////////////////////////////////////////////

AddressOptions::Iterator::Iterator() : mEntry(nullptr) {}

AddressOptions::Iterator::Iterator(AddressInfo* entry) : mEntry(entry) {}

AddressOptions::Iterator const& AddressOptions::Iterator::operator ++ () {
  if (mEntry != nullptr) {
    mEntry = mEntry->ai_next;
  }
  return *this;
}

bool AddressOptions::Iterator::operator == (Iterator const& iterator) const {
  return iterator.mEntry == mEntry;
}

bool AddressOptions::Iterator::operator != (Iterator const& iterator) const {
  return iterator.mEntry != mEntry;
}

AddressInfo const& AddressOptions::Iterator::operator * () const {
  return *mEntry;
}

////////////////////////////////////////////////////////////////////////////////

AddressOptions::AddressOptions(IPVersion version, SocketType type,
                               char const* host, char const* port)
  : mList(nullptr) {
  AddressInfo hints;
  memset(&hints, 0, sizeof hints);
  switch (version) {
  case IPv4: hints.ai_family = AF_INET; break;
  case IPv6: hints.ai_family = AF_INET6; break;
  case IPvX: hints.ai_family = AF_UNSPEC; break;
  }
  switch (type) {
  case TCP: hints.ai_socktype = SOCK_STREAM; break;
  case UDP: hints.ai_socktype = SOCK_DGRAM; break;
  }
  hints.ai_flags = AI_PASSIVE;
  int status = getaddrinfo(host, port, &hints, &mList);
  if (status != 0) {
    throw("Unable to 'getaddrinfo'.");
    mList = nullptr;
    return;
  }
}

AddressOptions::~AddressOptions() {
  if (mList != nullptr) {
    freeaddrinfo(mList);
  }
}

AddressOptions::Iterator AddressOptions::begin() const {
  return Iterator(mList);
}

AddressOptions::Iterator AddressOptions::end() const {
  return Iterator();
}

////////////////////////////////////////////////////////////////////////////////

static std::string addressToString(sockaddr* address, socklen_t addressLength) {
  char hostName[256];
  char serviceName[256];
  if (getnameinfo(address, addressLength,
                  hostName, 256, serviceName, 256,
                  NI_NUMERICHOST | NI_NUMERICSERV) == 0) {
    if (address->sa_family == AF_INET) {
      return std::string(hostName) + ":" + std::string(serviceName);
    }
    return "[" + std::string(hostName) + "]:" + std::string(serviceName);
  }
  return "<unknown>";
}

std::ostream& operator << (std::ostream& stream, AddressInfo const& info) {
  stream << addressToString(info.ai_addr, info.ai_addrlen);
  return stream;
}

////////////////////////////////////////////////////////////////////////////////

static bool makeNonBlocking(int socket) {
  int opts = fcntl(socket, F_GETFL);
  if (opts < 0) {
    return false;
  }
  opts = (opts | O_NONBLOCK);
  if (fcntl(socket, F_SETFL, opts) < 0) {
    return false;
  }
  return true;
}

////////////////////////////////////////////////////////////////////////////////

Socket::Socket()
  : mSocket(-1) {}

Socket::Socket(int fd, TLSContext&& tls)
  : mSocket(fd < 0 ? -1 : fd), mTls(std::move(tls)) {}

Socket::Socket(AddressInfo const& info, int maxQueue, TLSConfig const& tlsConfig)
  : mSocket(-1), mTls(tlsConfig) {
  mSocket = socket(info.ai_family, info.ai_socktype, 0);
  if (mSocket < 0) {
    mSocket = -1;
    throw std::runtime_error("socket() failed");
  }
  if (!makeNonBlocking(mSocket)) {
    close();
    throw std::runtime_error("makeNonBlocking() failed");
  }
  int on = 1;
  if (setsockopt(mSocket, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on))) {
    close();
    throw std::runtime_error("setsockopt() failed");
  }
  if (::bind(mSocket, info.ai_addr, info.ai_addrlen)) {
    close();
    throw std::runtime_error("bind() failed");
  }
  int status = ::listen(mSocket, maxQueue);
  if (status < 0) {
    close();
    throw std::runtime_error("listen() failed");
  }
}

Socket::Socket(Socket&& nbs)
  : mSocket(nbs.mSocket)
  , mTls(std::move(nbs.mTls)) {
  nbs.mSocket = -1;
}

Socket::~Socket() {
  close();
  mSocket = -2;
}

Socket const&
Socket::operator = (Socket&& nbs) {
  close();
  std::swap(mSocket, nbs.mSocket);
  std::swap(mTls, nbs.mTls);
  return *this;
}

Socket::operator bool() const {
  return mSocket >= 0;
}

void Socket::close() {
  if (mSocket >= 0) {
    ::close(mSocket);
  }
  mSocket = -1;
}

struct async_accept_impl {
  static constexpr int events = EV_READ;
  static constexpr decltype(::accept)* func = ::accept;
  static inline bool is_ready(int result) {
    return result >= 0 || (errno != EAGAIN && errno != EWOULDBLOCK);
  }
};
using async_accept = event::io_operation<async_accept_impl, decltype(::accept)>;
struct async_tls_handshake_impl {
  static constexpr int events = EV_READ | EV_WRITE;
  static constexpr decltype(::tls_handshake)* func = ::tls_handshake;
  static inline bool is_ready(int result) {
    return result == 0 || (result != TLS_WANT_POLLIN && result != TLS_WANT_POLLOUT);
  }
};
using async_tls_handshake = event::io_operation<async_tls_handshake_impl, decltype(::tls_handshake)>;
coro::task<Socket> Socket::async_accept(event::scheduler& s) {
  sockaddr_storage clientAddress;
  socklen_t clientAddressLength = sizeof(clientAddress);
  int client;
  do {
    client = co_await ::async_accept(s, mSocket, mSocket,
                                     (sockaddr*)&clientAddress,
                                     &clientAddressLength);
  } while (client == -1 && errno == EINTR);
  if (client == -1) {
    co_return Socket();
  }
  
  if (!makeNonBlocking(client)) {
    ::close(client);
    co_return Socket();
  }

  auto context = mTls.accept(client);
  /*if (0 != co_await ::async_tls_handshake(s, mSocket, context.context())) {
    co_return Socket();
  }
  std::cout << "hello!" << std::endl;*/
  co_return Socket(client, std::move(context));
}

struct async_tls_read_impl {
  static constexpr int events = EV_READ;
  static constexpr decltype(::tls_read)* func = ::tls_read;
  static inline bool is_ready(ssize_t result) {
    return result >= 0 || (result != TLS_WANT_POLLIN);
  }
};
using async_tls_read = event::io_operation<async_tls_read_impl, decltype(::tls_read)>;
coro::task<std::size_t> Socket::async_read(event::scheduler& s, char* buffer, std::size_t count) {
  auto result = co_await async_tls_read(s, mSocket, mTls.context(), buffer, count);
  if (result >= 0) {
    co_return result;
  }
  auto error_msg = tls_error(mTls.context());
  if (error_msg != nullptr) {
    throw std::runtime_error(error_msg);
  } else {
    throw std::runtime_error("tls_read failed");
  }
  co_return 0;
}

struct async_tls_write_impl {
  static constexpr int events = EV_WRITE;
  static constexpr decltype(::tls_write)* func = ::tls_write;
  static inline bool is_ready(ssize_t result) {
    return result >= 0 || (result != TLS_WANT_POLLOUT);
  }
};
using async_tls_write = event::io_operation<async_tls_write_impl, decltype(::tls_write)>;
coro::task<std::size_t> Socket::async_write(event::scheduler& s, char const* buffer, std::size_t count) {
  auto result = co_await async_tls_write(s, mSocket, mTls.context(), buffer, count);
  if (result >= 0) {
    co_return result;
  }
  auto error_msg = tls_error(mTls.context());
  if (error_msg != nullptr) {
    throw std::runtime_error(error_msg);
  } else {
    throw std::runtime_error("tls_read failed");
  }
  co_return 0;
}

std::string Socket::getLocalName() const {
  std::string local = "<void>";
  sockaddr_storage address;
  socklen_t addressLength = sizeof(address);
  if (getsockname(mSocket, reinterpret_cast<sockaddr*>(&address),
                  &addressLength) == 0) {
    assert(addressLength <= sizeof(address));
    local = addressToString(reinterpret_cast<sockaddr*>(&address),
                            addressLength);
  }
  return local;
}

std::string Socket::getRemoteName() const {
  std::string remote = "<void>";
  sockaddr_storage address;
  socklen_t addressLength = sizeof(address);
  if (getpeername(mSocket, reinterpret_cast<sockaddr*>(&address),
                  &addressLength) == 0) {
    assert(addressLength <= sizeof(address));
    remote = addressToString(reinterpret_cast<sockaddr*>(&address),
                             addressLength);
  }
  return remote;
}

////////////////////////////////////////////////////////////////////////////////

std::ostream& operator << (std::ostream& stream, Socket const& socket) {
  stream << socket.getLocalName() << " <-> " << socket.getRemoteName();
  return stream;
}

////////////////////////////////////////////////////////////////////////////////
