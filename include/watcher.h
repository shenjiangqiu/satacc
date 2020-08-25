#ifndef WATCHER_H
#define WATCHER_H
#include "component.h"
#include <deque>
#include "assign_wrap.h"
#include <tuple>
#include "cache_interface.h"
#include <fmt/format.h>
class watcher : public componet
{
    using pair_int_as = std::pair<int, assign_wrap *>;

private:
    unsigned read_size = 64;
    unsigned value_size = 64;
    unsigned process_size = 64;
    unsigned in_size = 64;
    unsigned in_mem_size = 64;
    /** 
     * from waiting_value_watcher_queue to out_memory_read_queue,
     * notice: each time only read one value, because they are not continued
     * notice: waiting_value_watcher_queue only contain each assign once,so when
     * the memory return, only if we recieve the last package, we push to the 
     * waiting_value_watcher_queue
    */
    bool from_value_to_out_mem();

    //from waiting_process_queue to out_send_queue
    bool from_process_to_out();

    /**
     * from waiting_read_watcher_queue to out_memory_read_queue
     * notice: we push 64 bytes request per cycle
     */
    bool from_read_watcher_to_mem();

    //from in_task to waiting_read_watcher_queue
    bool from_in_to_read();

    //from in_memory_resp_queuue to inside queue
    bool from_resp_to_insider();

    std::deque<cache_interface_req> waiting_value_watcher_queue; // int : remaining value to be processed
    std::deque<cache_interface_req> waiting_read_watcher_queue;  // int: remaing watcher to be read
    std::deque<cache_interface_req> waiting_process_queue;       // int : remaining watcher to be processed
    int next_clause = 0;
    /* data */
public:
    int next_c(int total)
    {
        return next_clause = (next_clause + 1) % total;
    }

    std::string get_line_trace() const override
    {
        return fmt::format("{}:{}", "watcher", std::to_string(get_busy_percent()));
    }
    std::string get_internal_size() const override
    {
        return fmt::format("name w_v w_d w_p in_ in_m out_m out_s out_write\n {} {} {} {} {} {} {} {} {}", "watcher",
                           waiting_value_watcher_queue.size(),
                           waiting_read_watcher_queue.size(),
                           waiting_process_queue.size(),
                           in_task_queue.size(),
                           in_memory_resp_queue.size(),
                           out_memory_read_queue.size(),
                           out_send_queue.size(),
                           out_write_watcher_list_queue.size());
    }
    bool empty() const override
    {
        bool e = waiting_value_watcher_queue.empty() and waiting_read_watcher_queue.empty() and waiting_process_queue.empty() and
                 in_task_queue.empty() and in_memory_resp_queue.empty() and out_memory_read_queue.empty() and out_send_queue.empty() and out_write_watcher_list_queue.empty();
        return e;
    }
    bool recieve_rdy() const
    {
        return in_task_queue.size() < in_size;
    }
    bool recieve_mem_rdy() const
    {
        return in_memory_resp_queue.size() < in_mem_size;
    }
    std::deque<cache_interface_req> in_task_queue;
    // interface
    std::deque<cache_interface_req> out_send_queue;

    std::deque<cache_interface_req> out_memory_read_queue; //int:flag
    std::deque<cache_interface_req> in_memory_resp_queue;

    std::deque<cache_interface_req> out_write_watcher_list_queue;

    bool cycle() override;
    watcher(uint64_t &t);
    ~watcher();
};

#endif