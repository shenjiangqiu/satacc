#include <catch.hpp>
#include <sjq_intersim.h>
#include <fmt/format.h>

TEST_CASE("intersim_pressure")
{
    uint64_t cycle;
    icnt m_icnt(cycle, 16, 8, 16);

    for (int i = 0; i < 100000; i++)
    {
        auto as = new assign_wrap(0, 100, 1, nullptr, 0);
        as->add_clause_addr(0, 1000);
        as->set_addr(1000);

        auto req = std::make_unique<cache_interface_req>(AccessType::ReadWatcherData, 0, 0, 0, as);
        m_icnt.in_reqs[0].push_back(std::move(req));
    }
}