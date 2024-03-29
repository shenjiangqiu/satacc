

#include "watcher.h"
#include <catch2/catch_test_macros.hpp>

#include "clause.h"

TEST_CASE("WatcherClause", "[advanced][core][componet]") {
  uint64_t current_cycle;
  clause c(current_cycle);
  watcher w(current_cycle);
  assign_wrap_factory af;
  auto new_wrap1 = af.create(10, 32, -1, nullptr, 0);
  auto new_wrap2 = af.create(11, 16, 2, new_wrap1, 1);
  new_wrap1->set_watcherlist_start_addr(3333);
  new_wrap1->add_clause_addr(0, 1);
  new_wrap1->add_clause_addr(10, 2);
  // new_wrap1->add_clause_addr(0, 100);

  new_wrap1->add_clause_literal(0, 0);
  new_wrap1->add_clause_literal(0, 2);
  new_wrap1->add_clause_literal(0, 3);
  new_wrap1->add_clause_literals_addr(0, 11);
  new_wrap1->add_clause_literals_addr(0, 12);
  new_wrap1->add_clause_literals_addr(0, 13);
  new_wrap1->add_generated_assignments(10, new_wrap2);
  new_wrap1->add_clause_addr(10, 200);
  new_wrap1->add_clause_literal(10, 0);
  new_wrap1->add_clause_literal(10, 2);
  new_wrap1->add_clause_literal(10, 3);
  new_wrap1->add_clause_literals_addr(10, 11);
  new_wrap1->add_clause_literals_addr(10, 12);
  new_wrap1->add_clause_literals_addr(10, 13);

  auto req1 = std::make_unique<cache_interface_req>(AccessType::ReadWatcherData,
                                                    0, 0, 0, new_wrap1);

  w.in_task_queue.push_back(std::move(req1));
  while (w.out_send_queue.size() == 0) {
    w.cycle();
    c.cycle();
    current_cycle++;
    if (!w.out_memory_read_queue.empty()) {
      w.in_memory_resp_queue.push_back(
          std::move(w.out_memory_read_queue.front()));
      w.out_memory_read_queue.pop_front();
    }
    if (!c.is_out_memory_read_queue_empty()) {
      c.push_to_in_memory_resp_queue(
          std::move(c.get_from_out_memory_read_queue()));
      c.pop_from_out_memory_read_queue();
    }
  }
  while (c.is_out_queue_empty()) {
    if (!w.out_send_queue.empty()) {
      c.push_to_in_task_queue(std::move(w.out_send_queue.front()));
      w.out_send_queue.pop_front();
    }
    w.cycle();
    c.cycle();
    if (!w.out_memory_read_queue.empty()) {
      w.in_memory_resp_queue.push_back(
          std::move(w.out_memory_read_queue.front()));
      w.out_memory_read_queue.pop_front();
    }
    if (!c.is_out_memory_read_queue_empty()) {
      c.push_to_in_memory_resp_queue(
          std::move(c.get_from_out_memory_read_queue()));
      c.pop_from_out_memory_read_queue();
    }
  }
  assert(*c.get_from_out_queue() ==
         cache_interface_req(AccessType::ReadWatcherData, 0, 0, 0, new_wrap2));
}