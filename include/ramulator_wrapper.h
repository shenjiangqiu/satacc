#ifndef RAMULATOR_WRAPPER_H
#define RAMULATOR_WRAPPER_H
#include "Cache.h"
#include "Memory.h"
#include "RamulatorConfig.h"
#include "Request.h"
#include "Statistics.h"
#include <component.h>
#include <ctype.h>
#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <queue>
#include <set>
#include <string>
#include <tuple>
#include <vector>
class ramulator_wrapper : public componet {
public:
  void send(uint64_t addr, bool is_read);
  void tick();
  void finish();
  ramulator_wrapper(const ramulator::Config &configs, int cacheline,
                    uint64_t &t_current_cycle);
  ~ramulator_wrapper();
  void call_back(ramulator::Request &req);
  bool empty() const override;
  std::string get_internal_size() const override;
  std::string get_line_trace() const override;

  uint64_t get() const { return out_queue.front(); }
  uint64_t pop() {
    auto ret = out_queue.front();
    out_queue.pop();
    return ret;
  }
  bool return_avaliable() const { return !out_queue.empty(); }
  [[nodiscard]] unsigned long long int getTotalRequestsRead() const {
    return total_requests_read;
  }
  [[nodiscard]] unsigned long long int getTotalRequestsWrite() const {
    return total_requests_write;
  }
  [[nodiscard]] unsigned long long int getCycleInMemory() const { return cycle_in_memory; }

protected:
  bool do_cycle() override;

private:
  unsigned long long total_requests_read=0;
  unsigned long long total_requests_write=0;
  unsigned long long cycle_in_memory=0;
  double tCK;
  unsigned long long outgoing_reqs = 0;
  std::queue<std::pair<uint64_t, bool>> in_queue;
  std::queue<uint64_t> out_queue;
  ramulator::MemoryBase *mem;
};

#endif