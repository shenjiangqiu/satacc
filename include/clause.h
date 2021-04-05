#ifndef CLAUSE_H
#define CLAUSE_H
#include "assign_wrap.h"
#include "cache_interface.h"
#include "component.h"
#include <deque>
#include <string>

#include <fmt/format.h>
class clause : public componet {
  using pair_int_as = std::pair<int, assign_wrap *>;
  using req_ptr = std::unique_ptr<cache_interface_req>;
  using req_ptr_q = std::deque<req_ptr>;
  using req_ptr_q_vec = std::vector<req_ptr_q>;

private:
  /* data */
  //unsigned ref_size = 64;
  unsigned data_size = 64;
  //unsigned value_size = 64;
  unsigned process_size = 64;
  unsigned in_size = 64;
  unsigned in_mem_size = 64;
  bool task_to_data_waiting();    // get the task and send to data waiting queue
  bool data_waiting_to_mem_out(); // get from data_waiting queue, and get clause
                                  // detail
  bool
  value_waiting_to_mem_out(); // get from value_waiting, and get value details
  bool mem_in_to_comp();      // to value_waiting and process waiting

  bool process_waiting_to_out(); // process the clause and send out

  std::deque<req_ptr> clause_data_read_waiting_queue;
  std::deque<req_ptr> clause_value_read_waiting_queue;
  std::deque<req_ptr> clause_process_waiting_queue;

public:
  std::string get_internal_size() const override {
    return fmt::format("name clause_d clause_v clause_p in_m in_ out_m out\n "
                       "{} {} {} {} {} {} {} {} {}",
                       "clause", clause_data_read_waiting_queue.size(),
                       clause_value_read_waiting_queue.size(),
                       clause_process_waiting_queue.size(),
                       in_memory_resp_queue.size(), in_task_queue.size(),
                       out_memory_read_queue.size(), out_queue.size(),
                       out_clause_write_queue.size());
  }

  std::string get_line_trace() const override {
    return fmt::format("{}\nidle_memory {}\nidle_task {}\nidle_other {}",
                       std::string("clause:") +
                           std::to_string(get_busy_percent()),
                       stall_on_going_request, stall_no_task, stall_other);
  }
  bool empty() const override {
    return clause_data_read_waiting_queue.empty() and
           clause_value_read_waiting_queue.empty() and
           clause_process_waiting_queue.empty() and
           in_memory_resp_queue.empty() and in_task_queue.empty() and
           out_memory_read_queue.empty() and out_queue.empty() and
           out_clause_write_queue.empty();
  }
  bool recieve_rdy() const { return in_task_queue.size() < in_size; }
  bool recieve_mem_rdy() const {
    return in_memory_resp_queue.size() < in_mem_size;
  }

  clause(uint64_t &tcurrent_cycle);
  ~clause();

private:
  std::deque<req_ptr> in_task_queue;
  std::deque<req_ptr> out_queue;

  std::deque<req_ptr> out_memory_read_queue; // int:flag
  std::deque<req_ptr> in_memory_resp_queue;

  // std::deque<req_ptr> out_watcher_list_write_queue;
  std::deque<req_ptr> out_clause_write_queue;

  int on_going_requests = 0;
  long long stall_on_going_request = 0;
  long long stall_no_task = 0;
  long long stall_other = 0;

public:
  int get_out_queue_size() const { return out_queue.size(); }
  int get_out_memory_read_queue_size() const {
    return out_memory_read_queue.size();
  }
  bool is_out_queue_empty() { return out_queue.empty(); }
  bool is_out_memory_read_queue_empty() {
    return out_memory_read_queue.empty();
  }
  bool is_out_clause_write_queue_empty() {
    return out_clause_write_queue.empty();
  }
  void push_to_in_task_queue(req_ptr req) {
    in_task_queue.push_back(std::move(req));
  }
  void push_to_in_memory_resp_queue(req_ptr req) {
    on_going_requests--;
    if (on_going_requests < 0) {
      throw "on going request can't be negtive";
    }
    in_memory_resp_queue.push_back(std::move(req));
  }
  req_ptr &get_from_out_queue() { return out_queue.front(); }
  req_ptr &get_from_out_memory_read_queue() {
    return out_memory_read_queue.front();
  }
  req_ptr &get_from_out_clause_write_queue() {
    return out_clause_write_queue.front();
  }

  void pop_from_out_queue() { out_queue.pop_front(); }
  void pop_from_out_memory_read_queue() {
    on_going_requests++;
    out_memory_read_queue.pop_front();
  }
  void pop_from_out_clause_write_queue() { out_clause_write_queue.pop_front(); }

protected:
  virtual bool do_cycle() override;
};

#endif