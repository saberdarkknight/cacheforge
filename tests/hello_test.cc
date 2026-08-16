#include "forgekv/forgekv.h"

#include <iostream>
#include <string_view>

int main() {
  constexpr std::string_view expected_name = "ForgeKV";

  if (forgekv::Name() != expected_name) {
    std::cerr << "Expected project name to be " << expected_name << '\n';
    return 1;
  }

  std::cout << "Hello from " << forgekv::Name() << "!\n";
  return 0;
}
