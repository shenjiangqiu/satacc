#ifndef CLAUSE_H
#define CLAUSE_H
#include "component.h"
#include <deque>
#include "assign_wrap.h"
#include "cache_interface.h"
class clause : public componet
{
    using pair_int_as = std::pair<int, assign_wrap *>;

private:
    /* data */
    int ref_size = 64;
    int data_size = 64;
    int value_size = 64;
    int process_size = 64;
    void task_to_data_waiting();     //get the task and send to data waiting queue
    void data_waiting_to_mem_out();  // get from data_waiting queue, and get clause detail
    void value_waiting_to_mem_out(); // get from value_waiting, and get value details
    void mem_in_to_comp();           //to value_waiting and process waiting

    void process_waiting_to_out(); //process the clause and send out

    std::deque<cache_interface_req> clause_data_read_waiting_queue;
    std::deque<cache_interface_req> clause_value_read_waiting_queue;
    std::deque<cache_interface_req> clause_process_waiting_queue;

public:
    void cycle() override;
    clause(/* args */);
    ~clause();

    std::deque<cache_interface_req> in_task_queue;
    std::deque<cache_interface_req> out_queue;

    std::deque<cache_interface_req> out_memory_read_queue; //int:flag
    std::deque<cache_interface_req> in_memory_resp_queue;
};

#endif