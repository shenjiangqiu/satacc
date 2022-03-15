#include <catch2/catch_test_macros.hpp>

#include <functional>
#include <vector>
#include <iostream>
#include <algorithm>
TEST_CASE("lambda", "[basic],[tool]")
{

    std::cout << "begin lambda test---" << std::endl;
    std::vector<std::function<void()>> functions;

    functions.push_back([]() {
        static int ii = 0;
        ii += 1;
        std::cout << "0 ii=" << ii << std::endl;
    });
    functions.push_back([]() {
        static int ii = 10;
        ii += 1;
        std::cout << "10 ii=" << ii << std::endl;
    });

    std::for_each(functions.begin(), functions.end(), [](auto f) {
        f();
    });
    std::for_each(functions.begin(), functions.end(), [](auto f) {
        f();
    });
    std::for_each(functions.begin(), functions.end(), [](auto f) {
        f();
    });
    std::cout << "end lambda test---" << std::endl;
}