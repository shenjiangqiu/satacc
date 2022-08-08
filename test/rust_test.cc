#include <acc.h>
#include <catch2/catch_test_macros.hpp>
#include <rusttools.h>
TEST_CASE("rust_test") {
  uint64_t cycle = 0;
  auto macc = acc(cycle);
}