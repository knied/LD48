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

Socket::Socket(int fd)
  : mSocket(fd < 0 ? -1 : fd) {}

Socket::Socket(AddressInfo const& info, int maxQueue)
  : mSocket(-1) {
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
  : mSocket(nbs.mSocket) {
  nbs.mSocket = -1;
}

Socket::~Socket() {
  close();
  mSocket = -2;
}

Socket const&
Socket::operator = (Socket&& nbs) {
  close();
  int tmp = mSocket;
  mSocket = nbs.mSocket;
  nbs.mSocket = tmp;
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

Socket Socket::accept() {
  sockaddr_storage clientAddress;
  socklen_t clientAddressLength = sizeof(clientAddress);
  int client;
  do {
    client = ::accept(mSocket,
                      (sockaddr*)&clientAddress,
                      &clientAddressLength);
  } while (client == -1 && errno == EINTR);
  if (client == -1) {
    return Socket();
  }
  
  if (!makeNonBlocking(client)) {
    ::close(client);
    return Socket();
  }
  
  return Socket(client);
}

bool Socket::receive(char* buffer, std::size_t* size) {
  ssize_t bytes = ::recv(mSocket, buffer, *size, 0);
  if (bytes <= 0) {
    return false;
  }
  *size = static_cast<std::size_t>(bytes);
  return true;
}

bool Socket::send(char const* buffer,
                             std::size_t bufferSize) {
  ssize_t offset = 0;
  while (offset < bufferSize) {
    ssize_t result = ::send(mSocket, buffer + offset, bufferSize - offset, 0);
    if (result < 0 && errno != EINTR && errno != EAGAIN) {
      std::cout << "WARNING: send failed after " << offset << " bytes." << std::endl;
      std::cout << "  errno(" << errno << "): " << strerror(errno) << std::endl;
      return false;
    }
    if (result > 0) {
      offset += result;
    }
  }
  return true;
}

bool Socket::send(std::uint8_t const* buffer,
                             std::size_t bufferSize) {
  return send(reinterpret_cast<char const*>(buffer), bufferSize);
}

bool Socket::send(std::string const& string) {
  return send(string.data(), string.size());
}

int Socket::descriptor() const {
  return mSocket;
}

////////////////////////////////////////////////////////////////////////////////

std::ostream& operator << (std::ostream& stream,
                           Socket const& socket) {
  int fd = socket.descriptor();
  std::string local = "<nobody>";
  std::string remote = "<nobody>";

  {
    sockaddr_storage address;
    socklen_t addressLength = sizeof(address);
    if (getsockname(fd, reinterpret_cast<sockaddr*>(&address),
                    &addressLength) == 0) {
      assert(addressLength <= sizeof(address));
      local = addressToString(reinterpret_cast<sockaddr*>(&address),
                              addressLength);
    }
  }

  {
    sockaddr_storage address;
    socklen_t addressLength = sizeof(address);
    if (getpeername(fd, reinterpret_cast<sockaddr*>(&address),
                    &addressLength) == 0) {
      assert(addressLength <= sizeof(address));
      remote = addressToString(reinterpret_cast<sockaddr*>(&address),
                               addressLength);
    }
  }

  stream << local << " <-> " << remote;
  
  return stream;
}

////////////////////////////////////////////////////////////////////////////////
