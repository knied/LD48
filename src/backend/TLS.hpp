////////////////////////////////////////////////////////////////////////////////

#ifndef BACKEND_TLS_HPP
#define BACKEND_TLS_HPP

////////////////////////////////////////////////////////////////////////////////

#include <tls.h>
#include <string>
#include <iostream>

////////////////////////////////////////////////////////////////////////////////

class TLSConfig final {
public:
  TLSConfig(std::string const& certFile,
            std::string const& keyFile) {
    mConfig = ::tls_config_new();
    if (mConfig == nullptr) {
      throw std::runtime_error("TLSConfig: tls_config_new failed");
    }
    /*if (::tls_config_set_ca_file(mConfig, caFile.c_str())) {
      ::tls_config_free(mConfig);
      throw std::runtime_error("TLSConfig: tls_config_set_ca_file failed");
      }*/
    if (::tls_config_set_cert_file(mConfig, certFile.c_str())) {
      ::tls_config_free(mConfig);
      throw std::runtime_error("TLSConfig: tls_config_set_cert_file failed");
    }
    if (::tls_config_set_key_file(mConfig, keyFile.c_str())) {
      ::tls_config_free(mConfig);
      throw std::runtime_error("TLSConfig: tls_config_set_key_file failed");
    }
  }
  ~TLSConfig() {
    ::tls_config_free(mConfig);
  }

  auto configure(tls* context) const {
    return ::tls_configure(context, mConfig);
  }

private:
  tls_config* mConfig;
};

////////////////////////////////////////////////////////////////////////////////

class TLSContext final {
private:
  TLSContext(tls* context)
    : mContext(context) {
  }
public:
  TLSContext() : mContext(nullptr) {}
  TLSContext(TLSConfig const& config) {
    mContext = ::tls_server();
    if (mContext == nullptr) {
      throw std::runtime_error("TLSServer: tls_server failed");
    }
    if (config.configure(mContext)) {
      tls_free(mContext);
      throw std::runtime_error("TLSServer: configure failed");
    }
  }
  TLSContext(TLSContext&& other)
    : mContext(other.mContext) {
    other.mContext = nullptr;
  }
  ~TLSContext() {
    if (mContext != nullptr) {
      tls_close(mContext);
      tls_free(mContext);
    }
  }
  TLSContext& operator = (TLSContext && other) {
    std::swap(mContext, other.mContext);
    return *this;
  }
  
  TLSContext(TLSContext const&) = delete;
  TLSContext& operator = (TLSContext const&) = delete;

  TLSContext accept(int socket) {
    tls* context = nullptr;
    if (::tls_accept_socket(mContext, &context, socket)) {
      throw std::runtime_error("TLSContext: tls_accept_socket failed");
    }
    return TLSContext(context);
  }

  tls* context() const { return mContext; }

private:
  tls* mContext;
};

////////////////////////////////////////////////////////////////////////////////

#endif // BACKEND_TLS_HPP

////////////////////////////////////////////////////////////////////////////////
