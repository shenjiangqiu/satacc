#include "acc.h"
#include "cache_interface.h"
#include "set_up.h"
#include <catch2/catch_test_macros.hpp>
#include <fstream>
#include <generate_config.h>
#include <iostream>

TEST_CASE("acc", "[advanced][core][componet]") {
  generate_config();
  uint64_t current_cycle = 0;
  auto m_acc = acc(current_cycle);
  m_acc.current_cycle = 0;
  SECTION("long") {
    auto as = generate_wrap();
    auto req = std::make_unique<cache_interface_req>(
        AccessType::ReadWatcherData, 0, 0, 0, as);
    m_acc.in_m_trail.push_back(std::move(req));
    bool print = true;
    while (!m_acc.empty()) {
      // std::cout << m_acc.get_internal_size() << std::endl;
      m_acc.cycle();
      // std::cout << m_acc.current_cycle << std::endl;

      if (print) {
        // std::cout << m_acc.get_internal_size();
      }
      m_acc.current_cycle++;
    }
    std::cout << "m_acc current cycle " << m_acc.current_cycle << std::endl;

    std::cout << m_acc.get_line_trace() << std::endl;
  }
  SECTION("short") {
    auto as = generate_wrap_short();
    auto req = std::make_unique<cache_interface_req>(
        AccessType::ReadWatcherData, 0, 0, 0, as);
    m_acc.in_m_trail.push_back(std::move(req));
    while (!m_acc.empty()) {
      // std::cout << m_acc.get_internal_size() << std::endl;
      m_acc.cycle();
      m_acc.current_cycle++;
    }
    std::cout << "m_acc current cycle " << m_acc.current_cycle << std::endl;

    std::cout << m_acc.get_line_trace() << std::endl;
  }
  SECTION("parallel") {
    auto as = generate_wrap_para();
    auto req1 = std::make_unique<cache_interface_req>(
        AccessType::ReadWatcherData, 0, 0, 0, as.first);
    auto req2 = std::make_unique<cache_interface_req>(
        AccessType::ReadWatcherData, 0, 0, 0, as.second);

    m_acc.in_m_trail.push_back(std::move(req1));
    m_acc.in_m_trail.push_back(std::move(req2));
    while (!m_acc.empty()) {
      // std::cout << m_acc.get_internal_size() << std::endl;
      REQUIRE_NOTHROW(m_acc.cycle());
      m_acc.current_cycle++;
    }
    std::cout << "m_acc current cycle " << m_acc.current_cycle << std::endl;

    std::cout << m_acc.get_line_trace() << std::endl;
  }
}

TEST_CASE("CPPTEST") {
  std::cout << "CPPTEST" << std::endl;
  int a = 10;
  int &b = a;

  std::cout << (unsigned long long)(int *)&a << std::endl;
  std::cout << (unsigned long long)(int *)&b << std::endl;
}