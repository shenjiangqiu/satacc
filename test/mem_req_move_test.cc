#include "acc.h"
#include "cache_interface.h"
#include "set_up.h"
#include <catch2/catch_test_macros.hpp>
#include <iostream>
#include <mem_req_interface.h>
#include <queue>

TEST_CASE("mem_req_test") {
  global_id = 0;
  auto req1 = std::make_unique<cache_interface_req>(AccessType::ReadClauseData,
                                                    0, 0, 0, nullptr);
  REQUIRE(req1->mid == 0);
  auto req2 = std::make_unique<cache_interface_req>(AccessType::ReadClauseData,
                                                    0, 0, 0, nullptr);
  REQUIRE(req2->mid == 1);
  auto req1_ptr = req1.get();
  auto req3 = std::move(req1);

  REQUIRE(req1_ptr == req3.get());
  REQUIRE(req3->mid == 0);
  std::queue<std::unique_ptr<cache_interface_req>> q;
  q.push(std::move(req2));
  std::queue<std::unique_ptr<cache_interface_req>> p;
  p.push(std::move(q.front()));
  q.pop();
  REQUIRE(p.front()->mid == 1);
}
TEST_CASE("mem_req_icnt") {
  uint64_t tcurrent_cycle = 0;
  auto m_icnt = new icnt_ring(tcurrent_cycle, 16, 3, 1, 0, 64, 3);
  global_id = 0;
  auto as1 = new assign_wrap(0, 10, 0, nullptr, 0);
  as1->set_watcherlist_start_addr(0x00010);
  auto as2 = new assign_wrap(0, 20, 0, nullptr, 0);
  as2->set_watcherlist_start_addr(0xfffff);
  auto req1 = std::make_unique<cache_interface_req>(AccessType::ReadWatcherData,
                                                    0, 1, 1, as1);
  REQUIRE(req1->mid == 0);
  auto req2 = std::make_unique<cache_interface_req>(AccessType::ReadWatcherData,
                                                    0, 0, 3, as2);
  REQUIRE(req2->mid == 1);

  m_icnt->in_reqs[1].push_back(std::move(req1));
  m_icnt->in_reqs[3].push_back(std::move(req2));
  int num_inflight = 2;
  while (true) {
    for (auto i = 0u; i < 8; i++) {
      if (!m_icnt->out_reqs[i].empty()) {
        // auto &req = m_icnt->out_reqs[i].front();
        std::cout << "req out from " << i << std::endl;
        num_inflight--;
        m_icnt->out_reqs[i].pop_front();
      }
    }
    if (num_inflight == 0) {
      break;
    }
    m_icnt->cycle();
    tcurrent_cycle++;
  }
}