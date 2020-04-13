#include <iostream>

int main(int argc, char const* argv[]) {
  std::cout << "Hello, world!" << std::endl;
  for (int i = 0; i < argc; ++i) {
    std::cout << "argv[" << i << "] = \"" << argv[i] << "\"" << std::endl;
  }
  return 0;
}
