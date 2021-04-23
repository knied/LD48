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

#if 0 // example
class MySocket : public WebSocket {
public:
  MySocket(char const* url) : WebSocket(url) {}
  virtual void onopen() {
    printf("onopen\n");
    fflush(stdout);
    std::string msg("WASM sent me!!!");
    send(msg.c_str(), msg.size());
  }
  virtual void onclose() {
    printf("onclose\n");
    fflush(stdout);
  }
  virtual void onerror(char const* buffer, unsigned int size) {
    std::string msg(buffer, size);
    printf("onerror: %s\n", msg.c_str());
    fflush(stdout);
  }
  virtual void onmessage(char const* buffer, unsigned int size) {
    std::string msg(buffer, size);
    printf("onmessage: %s\n", msg.c_str());
    fflush(stdout);
  }
};
#endif

#endif // FRONTEND_WEBSOCKET_HPP
