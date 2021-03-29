#ifndef FRONTEND_WEBSOCKET_HPP
#define FRONTEND_WEBSOCKET_HPP

class WebSocket {
public:
  WebSocket(char const* url);
  virtual ~WebSocket();
  void send(char const* buffer, unsigned int size);
  virtual void onopen();
  virtual void onclose();
  virtual void onerror(char const* buffer, unsigned int size);
  virtual void onmessage(char const* buffer, unsigned int size);
private:
  int mHandle;
};

#endif // FRONTEND_WEBSOCKET_HPP
