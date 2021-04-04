////////////////////////////////////////////////////////////////////////////////

#ifndef BACKEND_TLS_HPP
#define BACKEND_TLS_HPP

////////////////////////////////////////////////////////////////////////////////

#include <tls.h>
#include <string>
#include <iostream>

////////////////////////////////////////////////////////////////////////////////

namespace crypto {

class config final {
public:
  config(std::string const& certFile,
         std::string const& keyFile) {
    mConfig = ::tls_config_new();
    if (mConfig == nullptr) {
      throw std::runtime_error("TLSConfig: tls_config_new failed");
    }
    if (::tls_config_set_cert_file(mConfig, certFile.c_str())) {
      ::tls_config_free(mConfig);
      throw std::runtime_error("TLSConfig: tls_config_set_cert_file failed");
    }
    if (::tls_config_set_key_file(mConfig, keyFile.c_str())) {
      ::tls_config_free(mConfig);
      throw std::runtime_error("TLSConfig: tls_config_set_key_file failed");
    }
  }
  ~config() {
    ::tls_config_free(mConfig);
  }

  auto configure(tls* context) const {
    return ::tls_configure(context, mConfig);
  }

private:
  tls_config* mConfig;
};

class context final {
private:
  context(tls* context)
    : mContext(context) {
  }
public:
  context() : mContext(nullptr) {}
  context(config const& config) {
    mContext = ::tls_server();
    if (mContext == nullptr) {
      throw std::runtime_error("TLSServer: tls_server failed");
    }
    if (config.configure(mContext)) {
      tls_free(mContext);
      throw std::runtime_error("TLSServer: configure failed");
    }
  }
  context(context&& other)
    : mContext(other.mContext) {
    other.mContext = nullptr;
  }
  ~context() {
    if (mContext != nullptr) {
      tls_close(mContext);
      tls_free(mContext);
    }
  }
  context& operator = (context && other) {
    std::swap(mContext, other.mContext);
    return *this;
  }
  
  context(context const&) = delete;
  context& operator = (context const&) = delete;

  context accept(int socket) {
    tls* ctx = nullptr;
    if (::tls_accept_socket(mContext, &ctx, socket)) {
      throw std::runtime_error("TLSContext: tls_accept_socket failed");
    }
    return context(ctx);
  }

  tls* get_context() const { return mContext; }

private:
  tls* mContext;
};

} // namespace crypto

////////////////////////////////////////////////////////////////////////////////

#endif // BACKEND_TLS_HPP

////////////////////////////////////////////////////////////////////////////////
