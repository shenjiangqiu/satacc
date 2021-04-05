#include <Catch2/catch.hpp>
#include "watcher.h"
TEST_CASE("watcher_test", "[basic][core][componet]")
{
    uint64_t current_cycle;
    watcher m_watcher(current_cycle);
    assign_wrap_factory af;
    auto new_wrap1 = af.create(10, 32, -1, nullptr, 0);
    auto new_wrap2 = af.create(11, 16, 2, new_wrap1, 1);

    new_wrap1->add_clause_addr(0, 1);
    new_wrap1->add_clause_addr(10, 2);
    auto req1 = std::make_unique<cache_interface_req>(AccessType::ReadWatcherData, 0, 0, 0, new_wrap1);
    req1->as = new_wrap1;
    auto req2 = std::make_unique<cache_interface_req>(AccessType::ReadWatcherData, 0, 0, 0, new_wrap2);
    req2->as = new_wrap2;

    m_watcher.in_task_queue.push_back(std::move(req1));
    std::cout << m_watcher.get_internal_size() << std::endl; //1

    m_watcher.cycle(); //not the request is in waiting_read_watcher_queue
    current_cycle++;
    std::cout << m_watcher.get_internal_size() << std::endl; //in to w_d//2

    m_watcher.in_task_queue.push_back(std::move(req2));

    m_watcher.cycle(); //not the request is in out_memory_read_queue
    current_cycle++;
    std::cout << m_watcher.get_internal_size() << std::endl; //3
    REQUIRE(m_watcher.out_memory_read_queue.size() > 0);
    REQUIRE(m_watcher.out_memory_read_queue.front()->type == AccessType::ReadWatcherData);
    REQUIRE(m_watcher.out_memory_read_queue.front()->as == new_wrap1);

    m_watcher.out_memory_read_queue.pop_front();
    m_watcher.cycle();
    std::cout << m_watcher.get_internal_size() << std::endl; //4

    current_cycle++;
    REQUIRE(m_watcher.out_memory_read_queue.front()->type == AccessType::ReadWatcherData);
    REQUIRE(m_watcher.out_memory_read_queue.front()->as == new_wrap1);
    m_watcher.out_memory_read_queue.pop_front();
    m_watcher.cycle();
    std::cout << m_watcher.get_internal_size() << std::endl;

    current_cycle++;
    REQUIRE(m_watcher.out_memory_read_queue.size() > 0);

    REQUIRE(m_watcher.out_memory_read_queue.front()->type == AccessType::ReadWatcherData);
    REQUIRE(m_watcher.out_memory_read_queue.front()->as == new_wrap2);
    m_watcher.out_memory_read_queue.pop_front(); //pop wrap2

    m_watcher.in_memory_resp_queue.push_back(std::make_unique<cache_interface_req>(AccessType::ReadWatcherData, 0, 0, 0, new_wrap1));
    m_watcher.cycle(); //ignore this
    current_cycle++;
    m_watcher.in_memory_resp_queue.push_back(std::make_unique<cache_interface_req>(AccessType::ReadWatcherData, 16, 0, 0, new_wrap1));
    m_watcher.cycle(); //push to waiting value queue
    current_cycle++;
    m_watcher.cycle(); //push to send queue for watcher 0
    current_cycle++;
    REQUIRE(m_watcher.out_memory_read_queue.size() > 0);
    REQUIRE(m_watcher.out_memory_read_queue.front()->type == AccessType::ReadWatcherValue);
    REQUIRE(m_watcher.out_memory_read_queue.front()->watcherId == 0);
    REQUIRE(m_watcher.out_memory_read_queue.front()->as == new_wrap1);
    m_watcher.in_memory_resp_queue.push_back(std::make_unique<cache_interface_req>(AccessType::ReadWatcherData, 0, 0, 0, new_wrap2));
    m_watcher.out_memory_read_queue.pop_front(); //pop wrap2
    m_watcher.cycle();
    current_cycle++;
    REQUIRE(m_watcher.out_memory_read_queue.front()->type == AccessType::ReadWatcherValue);
    REQUIRE(m_watcher.out_memory_read_queue.front()->as == new_wrap1);
    REQUIRE(m_watcher.out_memory_read_queue.front()->watcherId == 1);
    REQUIRE(1 == 1);
}