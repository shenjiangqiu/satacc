#include <catch.hpp>
#include <fmt/format.h>
#include <string>
#include <iostream>
TEST_CASE("fmt_test")
{

    std::string ret;
    ret += (fmt::format("\n{}:{}\n", "cache_interface", 0.5) +
            fmt::format("c.num_hit:  {} ,c.num_hit_reserved  {}  ,c.num_miss {} ,c.num_res_fail {} \n",
                        1, 2, 3, 4) +
            fmt::format("the_hist {}\n", 5));
    std::cout << ret << std::endl;
}