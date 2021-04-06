#include <catch2/catch.hpp>

//this file make sure the in and expression, if the first if false, the second must not execute.

bool return_false()
{
    return false;
}
bool return_true()
{
    return true;
}
bool return_excpt()
{
    throw "error";
    return false;
}
TEST_CASE("and", "[tool][basic][cpp]")
{
    REQUIRE_NOTHROW(return_false() and return_excpt());
    REQUIRE_THROWS(return_true() and return_excpt());
}