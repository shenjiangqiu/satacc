#include <catch.hpp>
#include <ramulator_wrapper.h>
#include <RamulatorConfig.h>
#include <iostream>
using namespace ramulator;
TEST_CASE("ramulator_test")
{
    uint64_t cycle = 0;
    Config m_config("DDR4-config.cfg");
    m_config.set_core_num(1);
    auto mem = ramulator_wrapper(m_config, 64, cycle);

    for (int i = 0; i < 100; i++)
    {
        cycle++;
        mem.send(i * 1000, true);
        mem.cycle();
        std::cout << mem.get_internal_size() << std::endl;
        if (mem.return_avaliable())
        {
            auto ret = mem.pop();
            std::cout << "out: " << ret << " " << cycle << std::endl;
        }
    }
    while (!mem.empty())
    {
        cycle++;

        std::cout << mem.get_internal_size() << std::endl;
        mem.cycle();
        if (mem.return_avaliable())
        {
            auto ret = mem.pop();
            std::cout << "out: " << ret << " " << cycle << std::endl;
        }
    }
}