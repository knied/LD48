////////////////////////////////////////////////////////////////////////////////

#include "../fs.hpp"
#include <gtest/gtest.h>
#include <fstream>

////////////////////////////////////////////////////////////////////////////////

TEST(fs, cache_invalid_whitelist) {
  fs::cache c("bla");
  EXPECT_EQ(c.entries().size(), 0ull);
}

TEST(fs, cache_valid_whitelist_missing_files) {
  {
    std::ofstream wl("whitelist.ini",
                     std::ofstream::out);
    wl << "bla.txt" << std::endl;
    wl << "blub.html" << std::endl;
  }
  
  fs::cache c(".");
  EXPECT_EQ(c.entries().size(), 0ull);
}

TEST(fs, cache_valid_whitelist_valid_files) {
  {
    std::ofstream wl("whitelist.ini",
                     std::ofstream::out);
    wl << "test.txt" << std::endl;
    wl << "index.html" << std::endl;
  }
  {
    std::ofstream file("test.txt",
                       std::ofstream::out);
    file << "hello" << std::endl;
  }
  {
    std::ofstream file("index.html",
                       std::ofstream::out);
    file << "world!" << std::endl;
  }
  
  fs::cache c(".");
  EXPECT_EQ(c.entries().size(), 2ull);

  std::map<std::string, std::string> expectedContent {
    {"/test.txt", "hello\n"},
    {"/index.html", "world!\n"}
  };
  for (auto& e : c.entries()) {
    auto expected = expectedContent.find(e.first);
    auto content = std::string(e.second->data(), e.second->size());
    EXPECT_EQ(content, expected->second);
  }
}

TEST(fs, cache_find) {
  {
    std::ofstream wl("whitelist.ini",
                     std::ofstream::out);
    wl << "test.txt" << std::endl;
    wl << "index.html" << std::endl;
  }
  {
    std::ofstream file("test.txt",
                       std::ofstream::out);
    file << "hello" << std::endl;
  }
  {
    std::ofstream file("index.html",
                       std::ofstream::out);
    file << "world!" << std::endl;
  }

  fs::cache c(".");

  auto r0 = c.find("/test.txt");
  ASSERT_NE(r0, nullptr);
  EXPECT_EQ(r0->size(), 6ull);
  EXPECT_NE(r0->data(), nullptr);
  EXPECT_EQ(r0->location(), std::string("/test.txt"));
  EXPECT_EQ(r0->type(), http::content::mime_type::TEXT);

  auto r1 = c.find("/index.html");
  ASSERT_NE(r1, nullptr);
  EXPECT_EQ(r1->size(), 7ull);
  EXPECT_NE(r1->data(), nullptr);
  EXPECT_EQ(r1->location(), std::string("/index.html"));
  EXPECT_EQ(r1->type(), http::content::mime_type::HTML);

  auto r2 = c.find("/not_found.html");
  ASSERT_EQ(r2, nullptr);
}

////////////////////////////////////////////////////////////////////////////////
