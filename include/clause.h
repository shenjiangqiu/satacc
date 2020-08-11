#ifndef CLAUSE_H
#define CLAUSE_H
#include "component.h"
#include <deque>
#include "assign_wrap.h"
#include "cache_interface.h"
#include <string>
#include <fmt/core.h>
class clause : public componet
{
    using pair_int_as = std::pair<int, assign_wrap *>;

private:
    /* data */
    unsigned ref_size = 64;
    unsigned data_size = 64;
    unsigned value_size = 64;
    unsigned process_size = 64;
    unsigned in_size = 64;
    unsigned in_mem_size = 64;
    bool task_to_data_waiting();     //get the task and send to data waiting queue
    bool data_waiting_to_mem_out();  // get from data_waiting queue, and get clause detail
    bool value_waiting_to_mem_out(); // get from value_waiting, and get value details
    bool mem_in_to_comp();           //to value_waiting and process waiting

    bool process_waiting_to_out(); //process the clause and send out

    std::deque<cache_interface_req> clause_data_read_waiting_queue;
    std::deque<cache_interface_req> clause_value_read_waiting_queue;
    std::deque<cache_interface_req> clause_process_waiting_queue;
    uint64_t idle = 0;
    uint64_t busy = 0;

public:
    std::string get_internal_size()
    {
        return fmt::format("{} {} {} {} {} {} {}",
                           clause_data_read_waiting_queue.size(),
                           clause_value_read_waiting_queue.size(),
                           clause_process_waiting_queue.size(),
                           in_memory_resp_queue.size(),
                           in_task_queue.size(),
                           out_memory_read_queue.size(),
                           out_queue.size());
    }
    double get_busy_percent()
    {
        return double(busy) / double(busy + idle);
    }
    std::string get_line_trace()
    {
        return std::to_string(get_busy_percent());
    }
    bool empty()
    {
        return clause_data_read_waiting_queue.empty() and clause_value_read_waiting_queue.empty() and clause_process_waiting_queue.empty() and in_memory_resp_queue.empty() and
               in_task_queue.empty() and out_memory_read_queue.empty() and out_queue.empty();
    }
    bool recieve_rdy()
    {
        return in_task_queue.size() < in_size;
    }
    bool recieve_mem_rdy()
    {
        return in_memory_resp_queue.size() < in_mem_size;
    }
    bool cycle() override;
    clause(/* args */);
    ~clause();

    std::deque<cache_interface_req> in_task_queue;
    std::deque<cache_interface_req> out_queue;

    std::deque<cache_interface_req> out_memory_read_queue; //int:flag
    std::deque<cache_interface_req> in_memory_resp_queue;
};

#endif