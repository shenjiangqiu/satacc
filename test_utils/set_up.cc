
#include "assign_wrap.h"
#include <set_up.h>
assign_wrap *generate_wrap() {
  assign_wrap_factory af;
  auto new_wrap1 = af.create(10, 32, -1, nullptr, 0);

  for (int i = 0; i < 32; i++) {
    new_wrap1->add_blocker_addr(i, 200 + i);
  }
  auto new_wrap2 = af.create(11, 16, 2, new_wrap1, 1);
  for (int i = 0; i < 16; i++) {
    new_wrap2->add_blocker_addr(i, 400 + i);
  }
  auto new_wrap3 = af.create(11, 16, 2, new_wrap1, 1);
  for (int i = 0; i < 16; i++) {
    new_wrap3->add_blocker_addr(i, 600 + i);
  }
  new_wrap1->set_watcherlist_start_addr(3333);
  new_wrap1->add_clause_addr(0, 1000);
  new_wrap1->set_clause_id(0, 1);

  new_wrap1->add_clause_addr(10, 2000);
  new_wrap1->set_clause_id(10, 2);

  // new_wrap1->add_clause_addr(0, 100);

  new_wrap1->add_clause_literal(0, 0);
  new_wrap1->add_clause_literal(0, 2);
  new_wrap1->add_clause_literal(0, 3);
  new_wrap1->add_clause_literals_addr(0, 11);
  new_wrap1->add_clause_literals_addr(0, 12);
  new_wrap1->add_clause_literals_addr(0, 13);
  new_wrap1->add_generated_assignments(10, new_wrap2);
  new_wrap1->add_clause_addr(10, 200);
  new_wrap1->set_clause_id(10, 2);

  new_wrap1->add_clause_literal(10, 0);
  new_wrap1->add_clause_literal(10, 2);
  new_wrap1->add_clause_literal(10, 3);
  new_wrap1->add_clause_literals_addr(10, 11);
  new_wrap1->add_clause_literals_addr(10, 12);
  new_wrap1->add_clause_literals_addr(10, 13);

  new_wrap2->set_watcherlist_start_addr(4444);
  new_wrap2->add_clause_addr(0, 200);

  new_wrap2->add_clause_literal(0, 0);
  new_wrap2->add_clause_literal(0, 1);
  new_wrap2->add_clause_literal(0, 2);
  new_wrap2->add_clause_literals_addr(0, 22);
  new_wrap2->add_clause_literals_addr(0, 23);
  new_wrap2->add_clause_literals_addr(0, 24);

  new_wrap2->add_generated_assignments(0, new_wrap3);

  return new_wrap1;
}
assign_wrap *generate_wrap_short() {
  assign_wrap_factory af;
  auto new_wrap1 = af.create(10, 32, -1, nullptr, 0);

  for (int i = 0; i < 32; i++) {
    new_wrap1->add_blocker_addr(i, 200 + i);
  }
  auto new_wrap2 = af.create(11, 16, 2, new_wrap1, 1);
  for (int i = 0; i < 16; i++) {
    new_wrap2->add_blocker_addr(i, 400 + i);
  }
  auto new_wrap3 = af.create(11, 16, 2, new_wrap1, 1);
  for (int i = 0; i < 16; i++) {
    new_wrap3->add_blocker_addr(i, 600 + i);
  }
  new_wrap1->set_watcherlist_start_addr(3333);
  new_wrap1->add_clause_addr(0, 1000);
  new_wrap1->add_clause_addr(10, 2000);
  // new_wrap1->add_clause_addr(0, 100);

  new_wrap1->add_clause_literal(0, 0);
  new_wrap1->add_clause_literal(0, 2);
  new_wrap1->add_clause_literal(0, 3);
  new_wrap1->add_clause_literals_addr(0, 11);
  new_wrap1->add_clause_literals_addr(0, 12);
  new_wrap1->add_clause_literals_addr(0, 13);
  // new_wrap1->add_generated_assignments(10, new_wrap2);
  new_wrap1->add_clause_addr(10, 200);
  new_wrap1->add_clause_literal(10, 0);
  new_wrap1->add_clause_literal(10, 2);
  new_wrap1->add_clause_literal(10, 3);
  new_wrap1->add_clause_literals_addr(10, 11);
  new_wrap1->add_clause_literals_addr(10, 12);
  new_wrap1->add_clause_literals_addr(10, 13);

  new_wrap2->set_watcherlist_start_addr(4444);
  new_wrap2->add_clause_addr(0, 200);
  new_wrap2->add_clause_literal(0, 0);
  new_wrap2->add_clause_literal(0, 1);
  new_wrap2->add_clause_literal(0, 2);
  new_wrap2->add_clause_literals_addr(0, 22);
  new_wrap2->add_clause_literals_addr(0, 23);
  new_wrap2->add_clause_literals_addr(0, 24);

  //  new_wrap2->add_generated_assignments(0, new_wrap3);

  return new_wrap1;
}
std::pair<assign_wrap *, assign_wrap *> generate_wrap_para() {
  assign_wrap_factory af;
  auto new_wrap1 = af.create(10, 32, -1, nullptr, 0);

  new_wrap1->add_pushed_list(0, 10);
  new_wrap1->add_pushed_addr(0, 100110);
  new_wrap1->add_pushed_list(1, 20);
  new_wrap1->add_pushed_addr(1, 100120);
  new_wrap1->add_pushed_list(2, 10);
  new_wrap1->add_pushed_addr(2, 100110);
  new_wrap1->add_pushed_list(3, 10);
  new_wrap1->add_pushed_addr(3, 100110);
  new_wrap1->add_pushed_list(4, 10);
  new_wrap1->add_pushed_addr(4, 100110);
  new_wrap1->add_pushed_list(5, 10);
  new_wrap1->add_pushed_addr(5, 100110);
  new_wrap1->add_pushed_list(6, 10);
  new_wrap1->add_pushed_addr(6, 100110);
  new_wrap1->add_pushed_list(7, 10);
  new_wrap1->add_pushed_addr(7, 100110);
  new_wrap1->add_pushed_list(8, 10);
  new_wrap1->add_pushed_addr(8, 100110);
  new_wrap1->add_pushed_list(9, 10);
  new_wrap1->add_pushed_addr(9, 100190);

  for (int i = 0; i < 32; i++) {
    new_wrap1->add_blocker_addr(i, 200 + i);
  }
  auto new_wrap2 = af.create(11, 16, 2, new_wrap1, 1);
  for (int i = 0; i < 16; i++) {
    new_wrap2->add_blocker_addr(i, 400 + i);
  }
  new_wrap2->set_watcherlist_start_addr(111);
  auto new_wrap3 = af.create(11, 16, 2, new_wrap1, 1);
  for (int i = 0; i < 16; i++) {
    new_wrap3->add_blocker_addr(i, 600 + i);
  }
  new_wrap3->set_watcherlist_start_addr(10);
  auto new_wrap4 = af.create(11, 16, 2, new_wrap1, 1);
  for (int i = 0; i < 16; i++) {
    new_wrap4->add_blocker_addr(i, 600 + i);
  }

  new_wrap4->set_watcherlist_start_addr(2222);
  new_wrap1->set_watcherlist_start_addr(3333);

  new_wrap1->add_clause_addr(0, 1000);
  new_wrap1->add_clause_addr(10, 2000);
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

  new_wrap2->set_watcherlist_start_addr(4444);
  new_wrap2->add_clause_addr(0, 200);

  new_wrap2->add_clause_literal(0, 0);
  new_wrap2->add_clause_literal(0, 1);
  new_wrap2->add_clause_literal(0, 2);
  new_wrap2->add_clause_literals_addr(0, 22);
  new_wrap2->add_clause_literals_addr(0, 23);
  new_wrap2->add_clause_literals_addr(0, 24);

  new_wrap2->add_generated_assignments(0, new_wrap3);

  return std::make_pair(new_wrap1, new_wrap4);
}
