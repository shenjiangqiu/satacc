#ifndef WATCHER_H
#define WATCHER_H
#include "component.h"
#include <deque>
#include <assign_wrap.h>
#include <tuple>
#include "cache_interface.h"
class watcher : public componet
{
    using pair_int_as = std::pair<int, assign_wrap *>;

private:
    int read_size = 64;
    int value_size = 64;
    int process_size = 64;
    std::deque<cache_interface_req> waiting_value_watcher_queue; // int : remaining value to be processed
    std::deque<cache_interface_req> waiting_read_watcher_queue;  //int: remaing watcher to be read
    std::deque<cache_interface_req> waiting_process_queue;       // int : remaining watcher to be processed

    /* data */
public:
    std::deque<cache_interface_req> in_task_queue;
    // interface
    std::deque<cache_interface_req> out_send_queue;

    std::deque<cache_interface_req> out_memory_read_queue; //int:flag
    std::deque<cache_interface_req> in_memory_resp_queue;

    void cycle() override;
    watcher(/* args */);
    ~watcher();
};

#endif