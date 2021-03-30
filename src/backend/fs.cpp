////////////////////////////////////////////////////////////////////////////////

#include "fs.hpp"
#include <fstream>
#include <vector>
#include <iostream>

////////////////////////////////////////////////////////////////////////////////

namespace fs {

static std::vector<std::string>
readWhitelist(std::string const& fileName) {
  std::vector<std::string> names;
  std::ifstream whitelist(fileName.c_str());
  if (whitelist.is_open()) {
    std::string line;
    while (std::getline(whitelist, line)) {
      names.push_back(line);
    }
  }
  return names;
}

static std::vector<char>
readFile(std::string const& fileName) {
  std::vector<char> result;
  std::ifstream file(fileName.c_str(),
                     std::ios::binary | std::ios::ate);
  if (file.is_open()) {
    auto size = file.tellg();
    file.seekg(0, std::ios::beg);
    result.resize(size);
    file.read(result.data(), size);
  } else {
    throw std::runtime_error("File not found");
  }
  return result;
}

static http::content::mime_type
mimeTypeFromFileName(std::string const& fileName) {
  auto p = fileName.rfind('.');
  if (p != std::string::npos) {
    auto extension = fileName.substr(p + 1);
    if (extension == "html") {
      return http::content::mime_type::HTML;
    } else if (extension == "js") {
      return http::content::mime_type::JS;
    } else if (extension == "css") {
      return http::content::mime_type::CSS;
    } else if (extension == "wasm") {
      return http::content::mime_type::WASM;
    }
  }
  return http::content::mime_type::TEXT;
}

auto timestampOf(std::string const& fileName) {
  std::filesystem::path p = std::filesystem::current_path() / fileName;
  return std::filesystem::last_write_time(p);
}

resource::resource(std::string const& root, std::string const& location)
  : mType(mimeTypeFromFileName(location))
  , mRoot(root)
  , mLocation(location)
  , mData(readFile(mRoot + mLocation))
  , mTime(timestampOf(mRoot + mLocation)) {}

void resource::reload() {
  auto time = timestampOf(mRoot + mLocation);
  if (time != mTime) {
    std::cout << "File '" << mRoot << mLocation << "' changed on disk. Refreshing..." << std::endl;
    mData = readFile(mRoot + mLocation);
    mTime = time;
  }
}

cache::cache(std::string const& root) {
  auto names = readWhitelist(root + "/whitelist.ini");
  for (auto name : names) {
    auto location = "/" + name;
    auto fileName = root + location;
    try {
      mEntries.insert(
        std::make_pair(location, std::make_unique<resource>(root, location)));
    } catch (std::runtime_error& err) {
      std::cerr << "Unable to cache file: " << name << std::endl;
    }
  }
}

} // fs

////////////////////////////////////////////////////////////////////////////////
