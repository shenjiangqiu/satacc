#ifndef WATCHER_H
#define WATCHER_H
#include "component.h"
#include <deque>
#include "assign_wrap.h"
#include <tuple>
#include "cache_interface.h"
#include <fmt/format.h>
#include <set>
class watcher : public componet
{
    using pair_int_as = std::pair<int, assign_wrap *>;
    using req_ptr = std::unique_ptr<cache_interface_req>;
    using req_ptr_q = std::deque<req_ptr>;
    using req_ptr_q_vec = std::vector<req_ptr_q>;

private:
    unsigned outgoing_read_watcher_data = 0;
    unsigned outgoing_read_watcher_value = 0;
    //std::set<req_ptr &> waiting_watcher_value;
    //unsigned read_size = 64;
    //unsigned value_size = 64;
    //unsigned process_size = 64;
    unsigned in_size = 64;
    unsigned in_mem_size = 64;

    unsigned long long total_idle = 0;
    unsigned long long total_busy = 0;
    unsigned long long total_idle_no_task = 0;
    //unsigned long long total_idle_no_watcher_meta_data = 0;
    unsigned long long total_idle_no_watcher_data = 0;
    unsigned long long total_idle_no_value = 0;
    /** 
     * from waiting_value_watcher_queue to out_memory_read_queue,
     * notice: each time only read one value, because they are not continued
     * notice: waiting_value_watcher_queue only contain each assign once,so when
     * the memory return, only if we recieve the last package, we push to the 
     * waiting_value_watcher_queue
    */
    bool from_value_to_out_mem();
    bool from_watcher_meta_to_memory();
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

    std::deque<req_ptr> waiting_value_watcher_queue; // int : remaining value to be processed
    std::deque<req_ptr> waiting_read_watcher_queue;  // int: remaing watcher to be read
    std::deque<req_ptr> waiting_process_queue;       // int : remaining watcher to be processed
    std::deque<req_ptr> waiting_read_meta_queue;

    int next_clause = 0;

    int m_id;
    /* data */
public:
    static int global_id;

    int next_c(int total)
    {
        return next_clause = (next_clause + 1) % total;
    }

    std::string get_line_trace() const override
    {
        return fmt::format("{}:{} total_idle:{} total_busy:{} total_waiting_data:{} total_waiting_value:{} total_waiting_task:{} ", "watcher", std::to_string(get_busy_percent()),
                           total_idle,
                           total_busy,
                           total_idle_no_watcher_data,
                           total_idle_no_value,
                           total_idle_no_task);
    }
    std::string get_internal_size() const override
    {
        auto r = fmt::format("name w_meta w_v w_d w_p in_ in_m out_m out_s out_write\n {}-{} {} {} {} {} {} {} {} {} {}", "watcher", m_id,
                             waiting_read_meta_queue.size(),
                             waiting_value_watcher_queue.size(),
                             waiting_read_watcher_queue.size(),
                             waiting_process_queue.size(),
                             in_task_queue.size(),
                             in_memory_resp_queue.size(),
                             out_memory_read_queue.size(),
                             out_send_queue.size(),
                             out_write_watcher_list_queue.size());
        return r;
    }
    bool empty() const override
    {
        bool e = waiting_read_meta_queue.empty() and waiting_value_watcher_queue.empty() and waiting_read_watcher_queue.empty() and waiting_process_queue.empty() and
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
    std::deque<req_ptr> in_task_queue;
    // interface
    std::deque<req_ptr> out_send_queue;

    std::deque<req_ptr> out_memory_read_queue; //int:flag
    std::deque<req_ptr> in_memory_resp_queue;

    std::deque<req_ptr> out_write_watcher_list_queue;
    watcher(uint64_t &t);
    ~watcher() override;

protected:
    bool do_cycle() override;
};

#endif