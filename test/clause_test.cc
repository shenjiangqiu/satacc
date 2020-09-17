

#include "catch.hpp"
#include "watcher.h"

#include "clause.h"

TEST_CASE("Clause", "[basic][core][componet]")
{
    uint64_t current_cycle;

    clause c(current_cycle);

    assign_wrap_factory af;
    auto new_wrap1 = af.create(10, 32, -1, nullptr, 0);
    //auto new_wrap2 = af.create(11, 16, 2, new_wrap1, 1);

    new_wrap1->add_clause_addr(0, 1);
    new_wrap1->add_clause_addr(10, 2);

    new_wrap1->add_clause_literal(0, 0);
    new_wrap1->add_clause_literal(0, 2);
    new_wrap1->add_clause_literal(0, 3);

    new_wrap1->add_detail(0, 11);
    new_wrap1->add_detail(0, 12);
    new_wrap1->add_detail(0, 13);

    new_wrap1->add_clause_literal(10, 0);
    new_wrap1->add_clause_literal(10, 2);
    new_wrap1->add_clause_literal(10, 3);
    new_wrap1->add_detail(10, 11);
    new_wrap1->add_detail(10, 12);
    new_wrap1->add_detail(10, 13);

    auto req1 = std::make_unique<cache_interface_req>(AccessType::ReadWatcherData, 0, 0, 0, new_wrap1);

    auto req2 = std::make_unique<cache_interface_req>(AccessType::ReadWatcherData, 10, 0, 0, new_wrap1);

    c.in_task_queue.push_back(std::move(req1));
    c.in_task_queue.push_back(std::move(req2));
    c.cycle();
    current_cycle++;

    REQUIRE(c.out_memory_read_queue.size() == 0);
    c.cycle();
    current_cycle++;

    REQUIRE(c.out_memory_read_queue.size() == 1);
    REQUIRE(*c.out_memory_read_queue.front() == cache_interface_req(AccessType::ReadClauseData, 0, 0, 0, new_wrap1));
    c.out_memory_read_queue.pop_front();
    c.cycle();
    current_cycle++;

    REQUIRE(c.out_memory_read_queue.size() == 1);
    REQUIRE(*c.out_memory_read_queue.front() == cache_interface_req(AccessType::ReadClauseData, 10, 0, 0, new_wrap1));
    c.out_memory_read_queue.pop_front();

    c.cycle();
    current_cycle++;

    c.in_memory_resp_queue.push_back(std::make_unique<cache_interface_req>(AccessType::ReadClauseData, 0, 0, 0, new_wrap1));
    c.in_memory_resp_queue.push_back(std::make_unique<cache_interface_req>(AccessType::ReadClauseData, 10, 0, 0, new_wrap1));

    c.cycle();
    current_cycle++;
    //now in waiting value;
    c.cycle();
    current_cycle++;
    //now in out_mem;
    REQUIRE(c.out_memory_read_queue.size() == 1);
    REQUIRE(*c.out_memory_read_queue.front() == cache_interface_req(AccessType::ReadClauseValue, 0, 0, 0, new_wrap1));
    c.out_memory_read_queue.pop_front();
    c.cycle();
    current_cycle++;

    REQUIRE(c.out_memory_read_queue.size() == 1);
    REQUIRE(*c.out_memory_read_queue.front() == cache_interface_req(AccessType::ReadClauseValue, 0, 1, 0, new_wrap1));
    c.out_memory_read_queue.pop_front();
    c.cycle();
    current_cycle++;

    REQUIRE(c.out_memory_read_queue.size() == 1);
    REQUIRE(*c.out_memory_read_queue.front() == cache_interface_req(AccessType::ReadClauseValue, 0, 2, 0, new_wrap1));
    c.out_memory_read_queue.pop_front();
    c.cycle();
    current_cycle++;

    REQUIRE(c.out_memory_read_queue.size() == 1);
    REQUIRE(*c.out_memory_read_queue.front() == cache_interface_req(AccessType::ReadClauseValue, 10, 0, 0, new_wrap1));
    c.out_memory_read_queue.pop_front();
    c.cycle();
    current_cycle++;

    REQUIRE(c.out_memory_read_queue.size() == 1);
    REQUIRE(*c.out_memory_read_queue.front() == cache_interface_req(AccessType::ReadClauseValue, 10, 1, 0, new_wrap1));
    c.out_memory_read_queue.pop_front();
    c.cycle();
    current_cycle++;

    REQUIRE(c.out_memory_read_queue.size() == 1);
    REQUIRE(*c.out_memory_read_queue.front() == cache_interface_req(AccessType::ReadClauseValue, 10, 2, 0, new_wrap1));
    c.out_memory_read_queue.pop_front();
}