////////////////////////////////////////////////////////////////////////////////

#include "net.hpp"

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h>
#include <errno.h>
#include <iostream>
#include <sstream>
#include <cassert>
#include <string.h>

#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif
#define NET_DISABLE_SIGPIPE_ON_SOCKET

////////////////////////////////////////////////////////////////////////////////

struct async_accept_impl {
  static constexpr int events = EV_READ;
  static constexpr decltype(::accept)* func = ::accept;
  static inline bool is_ready(int result) {
    return result >= 0 || (errno != EAGAIN && errno != EWOULDBLOCK);
  }
};
using async_accept = event::io_operation<async_accept_impl, decltype(::accept)>;

struct async_read_impl {
  static constexpr int events = EV_READ;
  static constexpr decltype(::read)* func = ::read;
  static inline bool is_ready(ssize_t result) {
    return result >= 0 || (errno != EAGAIN && errno != EWOULDBLOCK);
  }
};
using async_read = event::io_operation<async_read_impl, decltype(::read)>;

struct async_write_impl {
  static constexpr int events = EV_WRITE;
  static constexpr decltype(::send)* func = ::send;
  static inline bool is_ready(ssize_t result) {
    return result >= 0 || (errno != EAGAIN && errno != EWOULDBLOCK);
  }
};
using async_write = event::io_operation<async_write_impl, decltype(::send)>;

struct async_tls_read_impl {
  static constexpr int events = EV_READ;
  static constexpr decltype(::tls_read)* func = ::tls_read;
  static inline bool is_ready(ssize_t result) {
    return result >= 0 || (result != TLS_WANT_POLLIN);
  }
};
using async_tls_read = event::io_operation<async_tls_read_impl, decltype(::tls_read)>;

struct async_tls_write_impl {
  static constexpr int events = EV_WRITE;
  static constexpr decltype(::tls_write)* func = ::tls_write;
  static inline bool is_ready(ssize_t result) {
    return result >= 0 || (result != TLS_WANT_POLLOUT);
  }
};
using async_tls_write = event::io_operation<async_tls_write_impl, decltype(::tls_write)>;

////////////////////////////////////////////////////////////////////////////////

namespace net {

////////////////////////////////////////////////////////////////////////////////

address_options::iterator::iterator() : mEntry(nullptr) {}

address_options::iterator::iterator(address_info* entry) : mEntry(entry) {}

address_options::iterator const& address_options::iterator::operator ++ () {
  if (mEntry != nullptr) {
    mEntry = mEntry->ai_next;
  }
  return *this;
}

bool address_options::iterator::operator == (iterator const& other) const {
  return other.mEntry == mEntry;
}

bool address_options::iterator::operator != (iterator const& other) const {
  return other.mEntry != mEntry;
}

address_info const& address_options::iterator::operator * () const {
  return *mEntry;
}

////////////////////////////////////////////////////////////////////////////////

address_options::address_options(ip_version version, socket_type type,
                                 char const* host, char const* port)
  : mList(nullptr) {
  address_info hints;
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
    throw std::runtime_error("error: Unable to 'getaddrinfo'");
    mList = nullptr;
    return;
  }
}

address_options::~address_options() {
  if (mList != nullptr) {
    freeaddrinfo(mList);
  }
}

address_options::iterator address_options::begin() const {
  return iterator(mList);
}

address_options::iterator address_options::end() const {
  return iterator();
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

#ifdef NET_DISABLE_SIGPIPE_ON_SOCKET
static bool disableSIGPIPE(int socket) {
  int on = 1;
  if (setsockopt(socket, SOL_SOCKET, SO_NOSIGPIPE, (char *)&on, sizeof(on))) {
    throw std::runtime_error("error: setsockopt(SO_NOSIGPIPE) failed");
    return false;
  }
  return true;
}
#endif

////////////////////////////////////////////////////////////////////////////////

socket::socket()
  : mSocket(-1) {}

socket::socket(int fd)
  : mSocket(fd < 0 ? -1 : fd) {}

socket::socket(address_info const& info, int maxQueue)
  : mSocket(-1) {
  mSocket = ::socket(info.ai_family, info.ai_socktype, 0);
  if (mSocket < 0) {
    mSocket = -1;
    throw std::runtime_error("error: socket() failed");
  }
  if (!makeNonBlocking(mSocket)) {
    close();
    throw std::runtime_error("error: makeNonBlocking() failed");
  }
  int on = 1;
  if (setsockopt(mSocket, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on))) {
    close();
    throw std::runtime_error("error: setsockopt(SO_REUSEADDR) failed");
  }
  if (::bind(mSocket, info.ai_addr, info.ai_addrlen)) {
    close();
    throw std::runtime_error("error: bind() failed");
  }
  int status = ::listen(mSocket, maxQueue);
  if (status < 0) {
    close();
    throw std::runtime_error("error: listen() failed");
  }
}

socket::socket(socket&& other)
  : mSocket(other.mSocket) {
  other.mSocket = -1;
}

socket::~socket() {
  close();
  mSocket = -2;
}
  
socket& socket::operator = (socket&& nbs) {
  close();
  std::swap(mSocket, nbs.mSocket);
  return *this;
}

socket::operator bool() const {
  return mSocket >= 0;
}

void socket::close() {
  if (mSocket >= 0) {
    ::close(mSocket);
  }
  mSocket = -1;
}

coro::task<socket>
socket::async_accept(event::scheduler& s) {
  sockaddr_storage clientAddress;
  socklen_t clientAddressLength = sizeof(clientAddress);
  int client;
  do {
    client = co_await ::async_accept(s, mSocket, mSocket,
                                     (sockaddr*)&clientAddress,
                                     &clientAddressLength);
  } while (client == -1 && errno == EINTR);
  if (client == -1) {
    co_return socket();
  }
  if (!makeNonBlocking(client)) {
    ::close(client);
    co_return socket();
  }
#ifdef NET_DISABLE_SIGPIPE_ON_SOCKET
  if (!disableSIGPIPE(client)) {
    ::close(client);
    co_return socket();
  }
#endif
  co_return socket(client);
}

coro::task<std::size_t>
socket::async_read(event::scheduler& s, void* buffer, size_t count) {
  auto result = co_await ::async_read(s, mSocket, mSocket, buffer, count);
  if (result >= 0) {
    co_return result;
  }
  auto error_msg = ::strerror(errno);
  std::stringstream ss;
  ss << "error: read failed with result " << result;
  if (error_msg != nullptr) {
    ss << std::endl << "note: \"" << error_msg << "\"";
  }
  throw std::runtime_error(ss.str());
  co_return 0;
}

coro::task<std::size_t>
socket::async_write(event::scheduler& s, void const* buffer, size_t count) {
  auto result = co_await ::async_write(s, mSocket, mSocket, buffer, count, MSG_NOSIGNAL);
  if (result >= 0) {
    co_return result;
  }
  auto error_msg = ::strerror(errno);
  std::stringstream ss;
  ss << "error: write failed with result " << result;
  if (error_msg != nullptr) {
    ss << std::endl << "note: \"" << error_msg << "\"";
  }
  throw std::runtime_error(ss.str());
  co_return 0;
}

std::string socket::local_name() const {
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

std::string socket::remote_name() const {
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

tls_socket::tls_socket(socket&& other, crypto::context&& tls)
  : socket(std::move(other)), mTls(std::move(tls)) {}

tls_socket::tls_socket(address_info const& info, int maxQueue,
                       crypto::config const& tlsConfig)
  : socket(info, maxQueue), mTls(tlsConfig) {}

tls_socket::tls_socket(tls_socket&& other)
  : socket(std::move(other))
  , mTls(std::move(other.mTls)) {}

tls_socket&
tls_socket::operator = (tls_socket&& other) {
  if (std::addressof(other) != this) {
    socket::operator = (std::move(other));
    std::swap(mTls, other.mTls);
  }
  return *this;
}

coro::task<tls_socket>
tls_socket::async_accept(event::scheduler& s) {
  auto client = co_await socket::async_accept(s);
  auto context = mTls.accept(client.fd());
  co_return tls_socket(std::move(client), std::move(context));
}

coro::task<std::size_t>
tls_socket::async_read(event::scheduler& s, char* buffer, std::size_t count) {
  auto result = co_await async_tls_read(s, fd(), mTls.get_context(), buffer, count);
  if (result >= 0) {
    co_return result;
  }
  auto error_msg = tls_error(mTls.get_context());
  std::stringstream ss;
  ss << "error: tls_read failed with result " << result;
  if (error_msg != nullptr) {
    ss << std::endl << "note: \"" << error_msg << "\"";
  }
  throw std::runtime_error(ss.str());
  co_return 0;
}

coro::task<std::size_t>
tls_socket::async_write(event::scheduler& s, char const* buffer, std::size_t count) {
  auto result = co_await async_tls_write(s, fd(), mTls.get_context(), buffer, count);
  if (result >= 0) {
    co_return result;
  }
  auto error_msg = tls_error(mTls.get_context());
  std::stringstream ss;
  ss << "error: tls_write failed with result " << result;
  if (error_msg != nullptr) {
    ss << std::endl << "note: \"" << error_msg << "\"";
  }
  throw std::runtime_error(ss.str());
  co_return 0;
}

////////////////////////////////////////////////////////////////////////////////

} // namespace net

////////////////////////////////////////////////////////////////////////////////

std::ostream& operator << (std::ostream& stream, net::address_info const& info) {
  stream << net::addressToString(info.ai_addr, info.ai_addrlen);
  return stream;
}

std::ostream& operator << (std::ostream& stream, net::socket const& socket) {
  stream << socket.local_name() << " <-> " << socket.remote_name();
  return stream;
}

std::ostream& operator << (std::ostream& stream, net::tls_socket const& socket) {
  stream << socket.local_name() << " <-TLS-> " << socket.remote_name();
  return stream;
}

////////////////////////////////////////////////////////////////////////////////
