////////////////////////////////////////////////////////////////////////////////

#ifndef BACKEND_SOCKET_HPP
#define BACKEND_SOCKET_HPP

////////////////////////////////////////////////////////////////////////////////

#include "event.hpp"
#include "tls.hpp"
#include <iostream>

////////////////////////////////////////////////////////////////////////////////

struct addrinfo;

////////////////////////////////////////////////////////////////////////////////

namespace net {

////////////////////////////////////////////////////////////////////////////////

enum ip_version {
  IPv4,
  IPv6,
  IPvX
}; // IPVersion

enum socket_type {
  TCP,
  UDP
}; // SocketType

using address_info = addrinfo;

////////////////////////////////////////////////////////////////////////////////

class address_options {
public:
  class iterator {
    address_info* mEntry;
    
  public:
    iterator();
    iterator(address_info* entry);
    
    iterator const& operator ++ ();
    bool operator == (iterator const& other) const;
    bool operator != (iterator const& other) const;
    address_info const& operator * () const;
  }; // Iterator
    
 private:
  address_info *mList;
  
 public:
  address_options(ip_version version, socket_type type,
                 char const* host, char const* port);
  ~address_options();
  
  iterator begin() const;
  iterator end() const;

 private:
  address_options(address_options const&);
  address_options const& operator = (address_options const&);
  
}; // AddressOptions

////////////////////////////////////////////////////////////////////////////////

class socket {
public:
  socket();
  socket(address_info const& info, int maxQueue);
  socket(socket&& nbs);
  socket(socket const&) = delete;
  socket& operator = (socket&& nbs);
  socket& operator = (socket const&) = delete;
  virtual ~socket();

  operator bool() const;
  void close();
  int fd() const { return mSocket; }

  coro::task<socket>
  async_accept(event::scheduler& s);
  coro::task<std::size_t>
  async_read(event::scheduler& s, void* buffer, size_t count);
  coro::task<std::size_t>
  async_write(event::scheduler& s, void const* buffer, size_t count);

  std::string local_name() const;
  std::string remote_name() const;

protected:
  socket(int fd);

private:
  int mSocket;
}; // socket

class tls_socket final : public socket {
public:
  tls_socket(address_info const& info, int maxQueue,
             crypto::config const& tlsConfig);
  tls_socket(tls_socket&& nbs);
  tls_socket(tls_socket const&) = delete;
  tls_socket& operator = (tls_socket&& nbs);
  tls_socket& operator = (tls_socket const&) = delete;
  ~tls_socket() = default;

  coro::task<tls_socket>
  async_accept(event::scheduler& s);
  coro::task<std::size_t>
  async_read(event::scheduler& s, char* buffer, std::size_t count);
  coro::task<std::size_t>
  async_write(event::scheduler& s, char const* buffer, std::size_t count);

private:
  tls_socket(socket&& other, crypto::context&& tls);

private:
  crypto::context mTls;
}; // tls_socket

////////////////////////////////////////////////////////////////////////////////

} // namespace net

////////////////////////////////////////////////////////////////////////////////

std::ostream& operator << (std::ostream& stream, net::address_info const& info);
std::ostream& operator << (std::ostream& stream, net::socket const& socket);
std::ostream& operator << (std::ostream& stream, net::tls_socket const& socket);

////////////////////////////////////////////////////////////////////////////////

#endif // BACKEND_SOCKET_HPP

////////////////////////////////////////////////////////////////////////////////
