#include "websocket.hpp"
#include "wasm.h"

WASM_IMPORT("websocket", "open")
int websocket_open(char const* url, void* context);
WASM_IMPORT("websocket", "close")
void websocket_close(int handle);
WASM_IMPORT("websocket", "send")
void websocket_send(int handle, char const* buffer, unsigned int size);

WASM_EXPORT("websocket_onopen")
void websocket_onopen(void* context) {
  WebSocket* socket = reinterpret_cast<WebSocket*>(context);
  socket->onopen();
}
WASM_EXPORT("websocket_onmessage")
void websocket_onmessage(void* context, char const* buffer, unsigned int size) {
  WebSocket* socket = reinterpret_cast<WebSocket*>(context);
  socket->onmessage(buffer, size);
}
WASM_EXPORT("websocket_onerror")
void websocket_onerror(void* context, char const* buffer, unsigned int size) {
  WebSocket* socket = reinterpret_cast<WebSocket*>(context);
  socket->onerror(buffer, size);
}
WASM_EXPORT("websocket_onclose")
void websocket_onclose(void* context) {
  WebSocket* socket = reinterpret_cast<WebSocket*>(context);
  socket->onclose();
}

WebSocket::WebSocket(char const* url)
  : mHandle(::websocket_open(url, this)) {}
WebSocket::~WebSocket() {
  ::websocket_close(mHandle);
}
void WebSocket::send(char const* buffer, unsigned int size) {
  ::websocket_send(mHandle, buffer, size);
}
void WebSocket::onopen() {}
void WebSocket::onclose() {}
void WebSocket::onerror(char const* /*buffer*/, unsigned int /*size*/) {}
void WebSocket::onmessage(char const* /*buffer*/, unsigned int /*size*/) {}
