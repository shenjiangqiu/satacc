#ifndef WATCHER_H
#define WATCHER_H
#include "component.h"
#include <deque>
#include "assign_wrap.h"
#include <tuple>
#include "cache_interface.h"
#include <fmt/core.h>
class watcher : public componet
{
    using pair_int_as = std::pair<int, assign_wrap *>;

private:
    unsigned read_size = 64;
    unsigned value_size = 64;
    unsigned process_size = 64;
    unsigned in_size = 64;
    unsigned in_mem_size = 64;

    std::deque<cache_interface_req> waiting_value_watcher_queue; // int : remaining value to be processed
    std::deque<cache_interface_req> waiting_read_watcher_queue;  //int: remaing watcher to be read
    std::deque<cache_interface_req> waiting_process_queue;       // int : remaining watcher to be processed
    uint64_t busy = 0;
    uint64_t idle = 0;
    /* data */
public:
    double get_busy_percent()
    {
        return double(busy) / double(busy + idle);
    }
    std::string get_line_trace()
    {
        return std::to_string(get_busy_percent());
    }
    std::string get_internal_size()
    {
        return fmt::format("{} {} {} {} {} {} {}",
                           waiting_value_watcher_queue.size(),
                           waiting_read_watcher_queue.size(),
                           waiting_process_queue.size(),
                           in_task_queue.size(),
                           in_memory_resp_queue.size(),
                           out_memory_read_queue.size(),
                           out_send_queue.size());
    }
    bool empty()
    {
        bool e = waiting_value_watcher_queue.empty() and waiting_read_watcher_queue.empty() and waiting_process_queue.empty() and
                 in_task_queue.empty() and in_memory_resp_queue.empty() and out_memory_read_queue.empty() and out_send_queue.empty();
        return e;
    }
    bool recieve_rdy()
    {
        return in_task_queue.size() < in_size;
    }
    bool recieve_mem_rdy()
    {
        return in_memory_resp_queue.size() < in_mem_size;
    }
    std::deque<cache_interface_req> in_task_queue;
    // interface
    std::deque<cache_interface_req> out_send_queue;

    std::deque<cache_interface_req> out_memory_read_queue; //int:flag
    std::deque<cache_interface_req> in_memory_resp_queue;

    bool cycle() override;
    watcher(uint64_t &t);
    ~watcher();
};

#endif