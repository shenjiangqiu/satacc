#ifndef WATCHER_LIST_WRITE_UNIT_H
#define WATCHER_LIST_WRITE_UNIT_H
#include "component.h"
#include <deque>
#include <string>
#include <fmt/format.h>
#include <fmt/ranges.h>
#include "cache_interface.h"
#include <map>
#include <array>
/*
    author: sjq

    currently, It will be too complicate to emulate the real behavior of changing the watcher list,
    we only emulate the behavior of two cache traffic:
    1, write the the current wather list( in this way, their might be deletion happen, so in hardware
    we need to maintain a location id for each watcher list current running.)
    2, write to the other watcher list, in this way, because we can proove there will be NO! deletion happened,

    Note:: in real hardware, we should use seperate unit in each watcher process unit. but here, for simplicity,
    we use only one unit to access all request.


    Note2:: be cause we only have limited wather list unit, this size can be comfirmed.
    and we my push to other watcher with uncertain size, so we need a dynamic reseve buffer.(merge the watcher list request to relife the l3 cache pressure.!!)

    !!!start to write cycle()! 
     _______________
    < You Can Do It >
     ---------------
      \            .    .     .
       \      .  . .     `  ,
        \    .; .  : .' :  :  : .
         \   i..`: i` i.i.,i  i .
          \   `,--.|i |i|ii|ii|i:
               UooU\.'@@@@@@`.||'
               \__/(@@@@@@@@@@)'
                    (@@@@@@@@)
                    `YY~~~~YY'
                     ||    ||


*/
class watcher_list_write_unit : public componet
{
private:
    /* data */
    //the map: the watcher list and current  number of the merge of 'the other' watcher list.
    std::map<int, int> other_map;
    std::deque<int> other_evict_queue;

    unsigned max_other_size;
    //the map: the map for the running watcher list to merge the write request.
    std::map<int, int> current_map;
    std::array<uint64_t, 8> evict_size_histo = {0};
    std::array<uint64_t, 8> evict_current_size_histo = {0};

public:
    watcher_list_write_unit(uint64_t &ct) : componet(ct) {}
    virtual std::string get_internal_size() const override
    {
        return fmt::format("name: {} in: {} out: {} current_map: {} other_map: {} ", "watcher_list_write_unit",
                           in_request.size(), out_mem_requst.size(), current_map.size(), other_map.size());
    }
    //virtual double get_busy_percent() const override;
    virtual std::string get_line_trace() const override
    {
        return fmt::format("name: {} busy_percent: {} \nevict_size_hist {} \nevict_current_hist {}",
                           "watcher_list_write_unit", get_busy_percent(), evict_size_histo, evict_current_size_histo);
    }
    //static uint64_t current_cycle;
    //bool busy;
    virtual bool empty() const override
    {
        return in_request.empty() and out_mem_requst.empty() and current_map.empty();
    }
    virtual bool cycle() override;

    //watcher_list_write_unit(uint64_t &current_cycle);
    ~watcher_list_write_unit();

    //the queue for in_request
    std::deque<cache_interface_req> in_request;
    std::deque<cache_interface_req> out_mem_requst;
};

#endif