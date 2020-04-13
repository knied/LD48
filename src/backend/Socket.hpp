////////////////////////////////////////////////////////////////////////////////

#ifndef BACKEND_SOCKET_HPP
#define BACKEND_SOCKET_HPP

////////////////////////////////////////////////////////////////////////////////

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
  int mSocket;
  
 public:
  Socket();
  Socket(AddressInfo const& info, int maxQueue);
  Socket(Socket&& nbs);
  Socket(Socket const&) = delete;
  Socket const& operator = (Socket const&) = delete;
  ~Socket();
  
  Socket const& operator = (Socket&& nbs);
  operator bool() const;
  
  void close();
  Socket accept();
  bool receive(char* buffer, std::size_t* length);
  bool send(char const* buffer, std::size_t length);
  bool send(std::uint8_t const* buffer, std::size_t length);
  bool send(std::string const& string);
  int descriptor() const;

 private:
  Socket(int fd);

}; // Socket

////////////////////////////////////////////////////////////////////////////////

std::ostream& operator << (std::ostream& stream, Socket const& socket);

////////////////////////////////////////////////////////////////////////////////

#endif // BACKEND_SOCKET_HPP

////////////////////////////////////////////////////////////////////////////////
