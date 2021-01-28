#ifndef SEQ_H
#define SEQ_H
#include <iostream>
#include <fmt/format.h>
#include <component.h>
#include <mem_req_interface.h>
#include <memory>
#include <queue>
#include <assert.h>
#include <sjqcache.h>
#include <ramulator_wrapper.h>
#include <cache_interface.h>
#include <ptr_copy.hpp>
#include <fmt/format.h>
//in this file, we implement the sequential pipeline

/* sequential pipeline
    push(): tasks from the trail
    get(): get the cycles of the time eclipse(will run the simulation when call get())
    this pipeline will handle the interal components
*/
class seq_pipeline
{
    using req_ptr = std::unique_ptr<cache_interface_req>;

public:
    seq_pipeline(int l3_size,
                 std::string dram_config,
                 uint64_t &current_cycle) : m_cache_interface(l3_size,
                                                              1, false, false, 1,
                                                              dram_config,
                                                              current_cycle),
                                            cycle(0), empty(true) {}

    void push(req_ptr &&req)
    {
        assert(empty);
        empty = false;
        in_reqs.push(std::move(req));
    }
    uint64_t get();

private:
    //sjq::cache m_l3_cache;
    cache_interface m_cache_interface;
    uint64_t cycle;
    bool empty;

    std::queue<req_ptr> in_reqs;
};

#endif