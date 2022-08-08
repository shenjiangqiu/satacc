#include "acc.h"
#include "cache_interface.h"
#include "set_up.h"
#include <addr_utiles.h>
#include <catch2/catch_test_macros.hpp>
#include <iostream>
#include <req_addr.h>
TEST_CASE() {
  uint64_t tcurrent_cycle = 0;
  auto m_icnt = new icnt_mesh(tcurrent_cycle, 16, 3, 1, 0, 64, 3);
  global_id = 0;
  auto as1 = new assign_wrap(0, 10, 0, nullptr, 0);
  as1->set_watcherlist_start_addr(0x00010);
  auto as2 = new assign_wrap(0, 20, 0, nullptr, 0);
  as2->set_watcherlist_start_addr(0xfffff);
  auto req1 = std::make_unique<cache_interface_req>(AccessType::ReadWatcherData,
                                                    0, 1, 1, as1);
  // REQUIRE(req1->mid == 0);
  auto req2 = std::make_unique<cache_interface_req>(AccessType::ReadWatcherData,
                                                    0, 0, 3, as2);
  std::cout << *req1 << std::endl;
  std::cout << *req2 << std::endl;
  // REQUIRE(req2->mid == 1);

  m_icnt->in_reqs[1].push_back(std::move(req1));
  auto mem_partition = get_partition_id_by_addr(get_addr_by_req(req2), 8);
  m_icnt->in_reqs[mem_partition].push_back(std::move(req2));
  int num_inflight = 2;
  while (true) {
    for (auto i = 0u; i < 8; i++) {
      if (!m_icnt->out_reqs[i].empty()) {
        auto &req = m_icnt->out_reqs[i].front();
        std::cout << "req out from " << i << std::endl;
        std::cout << *req << std::endl;

        num_inflight--;
        m_icnt->out_reqs[i].pop_front();
      }
    }
    for (auto i = 0u; i < 16; i++) {
      if (!m_icnt->out_reqs[i].empty()) {
        auto &req = m_icnt->out_reqs[i].front();
        std::cout << "resp out from " << i << std::endl;
        std::cout << *req << std::endl;
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