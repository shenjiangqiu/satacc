#include "catch.hpp"

#include "cache_interface.h"
#include "assign_wrap.h"
TEST_CASE("cache_interface", "[basic][core][componet]")
{

    //init

    assign_wrap_factory af;
    auto new_wrap1 = af.create(10, 32, -1, nullptr, 0);
    auto new_wrap2 = af.create(11, 16, 2, new_wrap1, 1);
    for (int i = 0; i < 32; i++)
    {
        new_wrap1->add_block_addr(i, 200 + i);
    }

    new_wrap1->add_clause_addr(0, 1);
    new_wrap1->add_clause_addr(10, 2);
    //new_wrap1->add_clause_addr(0, 100);
    new_wrap1->set_addr(3333);
    new_wrap1->add_clause_literal(0, 0);
    new_wrap1->add_clause_literal(0, 2);
    new_wrap1->add_clause_literal(0, 3);
    new_wrap1->add_detail(0, 11);
    new_wrap1->add_detail(0, 12);
    new_wrap1->add_detail(0, 13);
    new_wrap1->add_generated_assignments(10, new_wrap2);
    new_wrap1->add_clause_addr(10, 200);
    new_wrap1->add_clause_literal(10, 0);
    new_wrap1->add_clause_literal(10, 2);
    new_wrap1->add_clause_literal(10, 3);
    new_wrap1->add_detail(10, 11);
    new_wrap1->add_detail(10, 12);
    new_wrap1->add_detail(10, 13);
    //finish init
    uint64_t current_cycle;
    auto m_c = cache_interface(16, 1 << 16, 16, 16, 8, current_cycle);

    m_c.current_cycle = 0;
    SECTION("first test")
    {
        m_c.in_request_queues[0].push_back(std::make_unique<cache_interface_req>(AccessType::ReadClauseData, 0, 0, 0, new_wrap1));
        std::cout << "current cycle:" << current_cycle << std::endl;
        while (m_c.out_resp_queues[0].empty())
        {
            m_c.cycle();
            current_cycle++;
        }
        std::cout << *m_c.out_resp_queues[0].front() << std::endl;
        m_c.out_resp_queues[0].pop_front();
        std::cout << "current cycle:" << current_cycle << std::endl;

        m_c.in_request_queues[0].push_back(std::make_unique<cache_interface_req>(AccessType::ReadClauseData, 0, 0, 0, new_wrap1));
        std::cout << "current cycle:" << current_cycle << std::endl;
        while (m_c.out_resp_queues[0].empty())
        {
            m_c.cycle();
            current_cycle++;
        }
        std::cout << *m_c.out_resp_queues[0].front() << std::endl;
        m_c.out_resp_queues[0].pop_front();
        std::cout << "current cycle:" << current_cycle << std::endl;

        m_c.in_request_queues[0].push_back(std::make_unique<cache_interface_req>(AccessType::ReadWatcherData, 0, 0, 0, new_wrap1));
        std::cout << "current cycle:" << current_cycle << std::endl;
        while (m_c.out_resp_queues[0].empty())
        {
            m_c.cycle();
            current_cycle++;
        }
        std::cout << *m_c.out_resp_queues[0].front() << std::endl;
        m_c.out_resp_queues[0].pop_front();
        std::cout << "current cycle:" << current_cycle << std::endl;

        m_c.in_request_queues[0].push_back(std::make_unique<cache_interface_req>(AccessType::ReadWatcherData, 16, 0, 0, new_wrap1));
        std::cout << "current cycle:" << current_cycle << std::endl;
        while (m_c.out_resp_queues[0].empty())
        {
            m_c.cycle();
            current_cycle++;
        }
        std::cout << *m_c.out_resp_queues[0].front() << std::endl;
        m_c.out_resp_queues[0].pop_front();
        std::cout << "current cycle:" << current_cycle << std::endl;

        //bank burst test
        std::cout << " burst current cycle:" << current_cycle << std::endl;

        m_c.in_request_queues[0].push_back(std::make_unique<cache_interface_req>(AccessType::ReadWatcherData, 0, 0, 0, new_wrap1));
        m_c.in_request_queues[0].push_back(std::make_unique<cache_interface_req>(AccessType::ReadWatcherData, 16, 0, 0, new_wrap1));

        REQUIRE(m_c.out_resp_queues[0].empty() == true);
        while (m_c.out_resp_queues[0].empty())
        {
            m_c.cycle();
            current_cycle++;
        }
        std::cout << *m_c.out_resp_queues[0].front() << std::endl;
        m_c.out_resp_queues[0].pop_front();
        while (m_c.out_resp_queues[0].empty())
        {
            m_c.cycle();
            current_cycle++;
        }
        std::cout << *m_c.out_resp_queues[0].front() << std::endl;
        m_c.out_resp_queues[0].pop_front();

        std::cout << "current cycle:" << current_cycle << std::endl;
    }

    SECTION("Second test")
    {
        current_cycle = 0;
        std::cout << " burst current cycle:" << current_cycle << std::endl;

        m_c.in_request_queues[0].push_back(std::make_unique<cache_interface_req>(AccessType::ReadWatcherData, 0, 0, 0, new_wrap1));
        m_c.in_request_queues[0].push_back(std::make_unique<cache_interface_req>(AccessType::ReadWatcherData, 16, 0, 0, new_wrap1));

        REQUIRE(m_c.out_resp_queues[0].empty() == true);
        while (m_c.out_resp_queues[0].empty())
        {
            m_c.cycle();
            current_cycle++;
        }
        std::cout << *m_c.out_resp_queues[0].front() << std::endl;
        m_c.out_resp_queues[0].pop_front();
        while (m_c.out_resp_queues[0].empty())
        {
            m_c.cycle();
            current_cycle++;
        }
        std::cout << *m_c.out_resp_queues[0].front() << std::endl;
        m_c.out_resp_queues[0].pop_front();

        std::cout << "current cycle:" << current_cycle << std::endl;
    }
}
