////////////////////////////////////////////////////////////////////////////////

#ifndef BACKEND_SOCKET_HPP
#define BACKEND_SOCKET_HPP

////////////////////////////////////////////////////////////////////////////////

#include "event.hpp"
#include "TLS.hpp"
#include <iostream>

////////////////////////////////////////////////////////////////////////////////

struct addrinfo;

////////////////////////////////////////////////////////////////////////////////

enum IPVersion {
  IPv4,
  IPv6,
  IPvX
}; // IPVersion

enum SocketType {
  TCP,
  UDP
}; // SocketType

using AddressInfo = addrinfo;

////////////////////////////////////////////////////////////////////////////////

class AddressOptions {
public:
  class Iterator {
    AddressInfo* mEntry;
    
  public:
    Iterator();
    Iterator(AddressInfo* entry);
    
    Iterator const& operator ++ ();
    bool operator == (Iterator const& iterator) const;
    bool operator != (Iterator const& iterator) const;
    AddressInfo const& operator * () const;
  }; // Iterator
    
 private:
  AddressInfo *mList;
  
 public:
  AddressOptions(IPVersion version, SocketType type,
                 char const* host, char const* port);
  ~AddressOptions();
  
  Iterator begin() const;
  Iterator end() const;

 private:
  AddressOptions(AddressOptions const&);
  AddressOptions const& operator = (AddressOptions const&);
  
}; // AddressOptions

////////////////////////////////////////////////////////////////////////////////

std::ostream& operator << (std::ostream& stream, AddressInfo const& info);

////////////////////////////////////////////////////////////////////////////////

class Socket {
public:
  Socket();
  Socket(AddressInfo const& info, int maxQueue, TLSConfig const& tlsConfig);
  Socket(Socket&& nbs);
  Socket(Socket const&) = delete;
  Socket const& operator = (Socket const&) = delete;
  ~Socket();
  
  Socket const& operator = (Socket&& nbs);
  operator bool() const;
  void close();

  coro::task<Socket> async_accept(event::scheduler& s);
  coro::task<std::size_t> async_read(event::scheduler& s, char* buffer, std::size_t count);
  coro::task<std::size_t> async_write(event::scheduler& s, char const* buffer, std::size_t count);

  std::string getLocalName() const;
  std::string getRemoteName() const;

private:
  Socket(int fd, TLSContext&& tls);

private:
  int mSocket;
  TLSContext mTls;
}; // Socket

////////////////////////////////////////////////////////////////////////////////

std::ostream& operator << (std::ostream& stream, Socket const& socket);

////////////////////////////////////////////////////////////////////////////////

#endif // BACKEND_SOCKET_HPP

////////////////////////////////////////////////////////////////////////////////
