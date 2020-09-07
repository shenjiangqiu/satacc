#include <acc.h>
#include <mem_req_interface.h>
#include <icnt_wrapper.h>
#include <iostream>
#include <catch.hpp>
#include <utils/Options.h>
TEST_CASE("intersim", "[base],[component],[core]")
{
    global_init init;//will init the option into  global config

    ::icnt_wrapper_init();
    icnt_create(4, 4);
    icnt_init();
    //int argc;
    //char *argv[10];
    //Minisat::parseOptions(argc, argv);
    cache_interface_req req1(ReadType::ReadClauseData, 0, 0, 0, nullptr);
    icnt_push(0, 5, (void *)&req1, 8);
    auto ret = icnt_pop(5);
    unsigned c = 0;
    while (!ret)
    {
        c++;
        ret = icnt_pop(5);
        icnt_transfer();
    }
    std::cout << (*(cache_interface_req *)ret) << std::endl;
    std::cout << c << std::endl;
}