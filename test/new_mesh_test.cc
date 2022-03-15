#include <catch2/catch_test_macros.hpp>
#include "acc.h"
#include "set_up.h"
#include "cache_interface.h"
#include <iostream>
#include <mem_req_interface.h>
#include <queue>
#include <new_intersim_wrapper.h>

TEST_CASE("mesh")
{
    uint64_t cycle = 0;
    icnt_base *i = new icnt_mesh(cycle, 16, 5, 1, 0, 64, 5);
    SECTION("basic")
    {
        auto req = std::make_unique<cache_interface_req>(AccessType::ReadClauseData, 0, 0, 0, nullptr);
        auto req2 = std::make_unique<cache_interface_req>(AccessType::ReadClauseData, 0, 0, 0, nullptr);

        req->icnt_from = 0;
        req->icnt_to = 13;
        req2->icnt_from = 13;
        req2->icnt_to = 0;

        i->in_reqs[0].push_back(std::move(req));
        i->in_reqs[13].push_back(std::move(req2));
        while (i->out_reqs[13].empty() or i->out_reqs[0].empty())
        {
            i->cycle();
            cycle++;
            /* code */
        }
        auto req3 = std::move(i->out_reqs[13].front());
        i->out_reqs[13].pop_front();
        i->out_reqs[0].pop_front();
        REQUIRE(i->empty() == true);
        std::cout << cycle << std::endl;
    }
    SECTION("pressure")
    {
        for (int ii = 0; ii < 1000; ii++)
        {
            auto reqt = std::make_unique<cache_interface_req>(AccessType::ReadClauseData, 0, 0, 0, nullptr);
            reqt->icnt_from = ii % 16;
            reqt->icnt_to = (ii + 33) / 3 % 16;
            if (reqt->icnt_from == reqt->icnt_to)
                reqt->icnt_to++;
            i->in_reqs[ii % 16].push_back(std::move(reqt));
        }

        auto remain = 1000;
        while (remain)
        {
            assert(!i->empty());
            i->cycle();
            cycle++;
            for (int ii = 0; ii < 16; ii++)
            {
                if (!i->out_reqs[ii].empty())
                {
                    auto &req = i->out_reqs[ii].front();
                    std::cout << "out: " << req->mid << std::endl;
                    std::cout.flush();
                    i->out_reqs[ii].pop_front();
                    remain--;
                }
            }
            /* code */
        }
        std::cout << cycle << std::endl;
    }
}