#include <memory>
#include <iostream>
#include <Catch2/catch.hpp>
TEST_CASE("UNIPTR_TEST", "[basic],[tools]")
{
    auto a = std::unique_ptr<int>(new int(5));
    auto b = std::move(a);
    //REQUIRE_THROWS(std::cout << *a << std::endl);
}

