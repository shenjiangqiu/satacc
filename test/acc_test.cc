#include "catch.hpp"
#include "acc.h"
#include "set_up.h"
#include "cache_interface.h"
#include <iostream>

TEST_CASE("acc", "[advanced][core][componet]")
{
    uint64_t current_cycle = 0;
    auto m_acc = acc(4, 4, current_cycle);
    m_acc.current_cycle = 0;
    SECTION("long")
    {
        auto as = generate_wrap();
        auto req = cache_interface_req(ReadType::ReadWatcher, 0, 0, 0, as);
        m_acc.in_m_trail.push_back(req);
        while (!m_acc.empty())
        {
            //std::cout << m_acc.get_internal_size() << std::endl;
            m_acc.cycle();
            m_acc.current_cycle++;
        }
        std::cout << "m_acc current cycle " << m_acc.current_cycle << std::endl;

        std::cout << m_acc.get_line_trace() << std::endl;
    }
    SECTION("short")
    {
        auto as = generate_wrap_short();
        auto req = cache_interface_req(ReadType::ReadWatcher, 0, 0, 0, as);
        m_acc.in_m_trail.push_back(req);
        while (!m_acc.empty())
        {
            //std::cout << m_acc.get_internal_size() << std::endl;
            m_acc.cycle();
            m_acc.current_cycle++;
        }
        std::cout << "m_acc current cycle " << m_acc.current_cycle << std::endl;

        std::cout << m_acc.get_line_trace() << std::endl;
    }
    SECTION("parallel")
    {
        auto as = generate_wrap_para();
        auto req1 = cache_interface_req(ReadType::ReadWatcher, 0, 0, 0, as.first);
        auto req2 = cache_interface_req(ReadType::ReadWatcher, 0, 0, 0, as.second);

        m_acc.in_m_trail.push_back(req1);
        m_acc.in_m_trail.push_back(req2);
        while (!m_acc.empty())
        {
            //std::cout << m_acc.get_internal_size() << std::endl;
            m_acc.cycle();
            m_acc.current_cycle++;
        }
        std::cout << "m_acc current cycle " << m_acc.current_cycle << std::endl;

        std::cout << m_acc.get_line_trace() << std::endl;
    }
}