#include "catch.hpp"
#include "watcher.h"
TEST_CASE("watcher_test", "[basic][core][componet]")
{
    watcher m_watcher;
    assign_wrap_factory af;
    auto new_wrap1 = af.create(10, 32, -1, nullptr, 0);
    auto new_wrap2 = af.create(11, 16, 2, new_wrap1, 1);

    new_wrap1->add_modified_list(0, 1);
    new_wrap1->add_modified_list(10, 2);
    cache_interface_req req1(ReadType::ReadWatcher, 0, 0, 0, new_wrap1);
    req1.as = new_wrap1;
    cache_interface_req req2(ReadType::ReadWatcher, 0, 0, 0, new_wrap2);
    req2.as = new_wrap2;
    m_watcher.in_task_queue.push_back(req1);

    m_watcher.cycle(); //not the request is in waiting_read_watcher_queue

    m_watcher.in_task_queue.push_back(req2);

    m_watcher.cycle(); //not the request is in out_memory_read_queue
    REQUIRE(m_watcher.out_memory_read_queue.size() > 0);
    REQUIRE(m_watcher.out_memory_read_queue.front().type == ReadType::ReadWatcher);
    REQUIRE(m_watcher.out_memory_read_queue.front().as == new_wrap1);

    m_watcher.out_memory_read_queue.pop_front();
    m_watcher.cycle();
    REQUIRE(m_watcher.out_memory_read_queue.front().type == ReadType::ReadWatcher);
    REQUIRE(m_watcher.out_memory_read_queue.front().as == new_wrap1);
    m_watcher.out_memory_read_queue.pop_front();
    m_watcher.cycle();
    REQUIRE(m_watcher.out_memory_read_queue.front().type == ReadType::ReadWatcher);
    REQUIRE(m_watcher.out_memory_read_queue.front().as == new_wrap2);
    m_watcher.out_memory_read_queue.pop_front(); //pop wrap2

    m_watcher.in_memory_resp_queue.push_back(cache_interface_req(ReadType::ReadWatcher, 0, 0, 0, new_wrap1));
    m_watcher.cycle(); //ignore this
    m_watcher.in_memory_resp_queue.push_back(cache_interface_req(ReadType::ReadWatcher, 16, 0, 0, new_wrap1));
    m_watcher.cycle(); //push to waiting value queue
    m_watcher.cycle(); //push to send queue for watcher 0
    REQUIRE(m_watcher.out_memory_read_queue.size() > 0);
    REQUIRE(m_watcher.out_memory_read_queue.front().type == ReadType::WatcherReadValue);
    REQUIRE(m_watcher.out_memory_read_queue.front().watcherId == 0);
    REQUIRE(m_watcher.out_memory_read_queue.front().as == new_wrap1);
    m_watcher.in_memory_resp_queue.push_back(cache_interface_req(ReadType::ReadWatcher, 0, 0, 0, new_wrap2));
    m_watcher.out_memory_read_queue.pop_front(); //pop wrap2
    m_watcher.cycle();
    REQUIRE(m_watcher.out_memory_read_queue.front().type == ReadType::WatcherReadValue);
    REQUIRE(m_watcher.out_memory_read_queue.front().as == new_wrap1);
    REQUIRE(m_watcher.out_memory_read_queue.front().watcherId == 1);
    REQUIRE(1 == 1);
}