////////////////////////////////////////////////////////////////////////////////

#ifndef BACKEND_FS_HPP
#define BACKEND_FS_HPP

////////////////////////////////////////////////////////////////////////////////

#include <map>
#include "http.hpp"

namespace fs {

class resource final : public http::content {
public:
  resource(std::string const& location,
           std::vector<char>&& data);
  resource(resource const&) = delete;
  resource(resource&&) = delete;
  resource& operator = (resource const&) = delete;
  resource& operator = (resource&&) = delete;
  
  // http::content
  virtual std::size_t size() const override {
    return mData.size();
  }
  virtual char const* data() const override {
    return mData.data();
  }
  virtual std::string const& location() const override {
    return mLocation;
  }
  virtual mime_type type() const override {
    return mType;
  }
private:
  http::content::mime_type mType = http::content::mime_type::TEXT;
  std::string mLocation;
  std::vector<char> mData;
};

class cache final {
public:
  using cache_entries = std::map<std::string, std::unique_ptr<resource>>;

  cache(std::string const& root);
  cache(cache const&) = delete;
  cache(cache&&) = delete;
  cache& operator = (cache const&) = delete;
  cache& operator = (cache&&) = delete;

  resource const*
  find(std::string const& name) const {
    auto it = mEntries.find(name);
    if (it == mEntries.end()) {
      return nullptr;
    }
    return it->second.get();
  }
  cache_entries const& entries() const {
    return mEntries;
  }

private:
  cache_entries mEntries;
};

} // fs

////////////////////////////////////////////////////////////////////////////////

#endif // BACKEND_FS_HPP

////////////////////////////////////////////////////////////////////////////////
