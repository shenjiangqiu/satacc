

#include "catch.hpp"
#include "watcher.h"

#include "clause.h"

TEST_CASE("Clause", "Clause")
{
    clause c;

    assign_wrap_factory af;
    auto new_wrap1 = af.create(10, 32, -1, nullptr, 0);
    auto new_wrap2 = af.create(11, 16, 2, new_wrap1, 1);

    new_wrap1->add_modified_list(0, 1);
    new_wrap1->add_modified_list(10, 2);

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

    cache_interface_req req1(ReadType::ReadWatcher, 0, 0, 0, new_wrap1);

    cache_interface_req req2(ReadType::ReadWatcher, 10, 0, 0, new_wrap1);

    c.in_task_queue.push_back(req1);
    c.in_task_queue.push_back(req2);
    c.cycle();
    REQUIRE(c.out_memory_read_queue.size() == 0);
    c.cycle();
    REQUIRE(c.out_memory_read_queue.size() == 1);
    REQUIRE(c.out_memory_read_queue.front() == cache_interface_req(ReadType::ReadClauseData, 0, 0, 0, new_wrap1));
    c.out_memory_read_queue.pop_front();
    c.cycle();
    REQUIRE(c.out_memory_read_queue.size() == 1);
    REQUIRE(c.out_memory_read_queue.front() == cache_interface_req(ReadType::ReadClauseData, 10, 0, 0, new_wrap1));
    c.out_memory_read_queue.pop_front();

    c.cycle();
    c.in_memory_resp_queue.push_back(cache_interface_req(ReadType::ReadClauseData, 0, 0, 0, new_wrap1));
    c.in_memory_resp_queue.push_back(cache_interface_req(ReadType::ReadClauseData, 10, 0, 0, new_wrap1));

    c.cycle(); //now in waiting value;
    c.cycle(); //now in out_mem;
    REQUIRE(c.out_memory_read_queue.size() == 1);
    REQUIRE(c.out_memory_read_queue.front() == cache_interface_req(ReadType::ReadClauseValue, 0, 0, 0, new_wrap1));
    c.out_memory_read_queue.pop_front();
    c.cycle();
    REQUIRE(c.out_memory_read_queue.size() == 1);
    REQUIRE(c.out_memory_read_queue.front() == cache_interface_req(ReadType::ReadClauseValue, 0, 1, 0, new_wrap1));
    c.out_memory_read_queue.pop_front();
    c.cycle();
    REQUIRE(c.out_memory_read_queue.size() == 1);
    REQUIRE(c.out_memory_read_queue.front() == cache_interface_req(ReadType::ReadClauseValue, 0, 2, 0, new_wrap1));
    c.out_memory_read_queue.pop_front();
    c.cycle();
    REQUIRE(c.out_memory_read_queue.size() == 1);
    REQUIRE(c.out_memory_read_queue.front() == cache_interface_req(ReadType::ReadClauseValue, 10, 0, 0, new_wrap1));
    c.out_memory_read_queue.pop_front();
    c.cycle();
    REQUIRE(c.out_memory_read_queue.size() == 1);
    REQUIRE(c.out_memory_read_queue.front() == cache_interface_req(ReadType::ReadClauseValue, 10, 1, 0, new_wrap1));
    c.out_memory_read_queue.pop_front();
    c.cycle();
    REQUIRE(c.out_memory_read_queue.size() == 1);
    REQUIRE(c.out_memory_read_queue.front() == cache_interface_req(ReadType::ReadClauseValue, 10, 2, 0, new_wrap1));
    c.out_memory_read_queue.pop_front();
}