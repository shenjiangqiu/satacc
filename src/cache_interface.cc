#include "cache_interface.h"
#include "component.h"
void cache_interface::cycle()
{
    busy = false;
    //from in to resp or missq
    from_dramresp_to_resp();
    from_miss_q_to_dram();
    from_in_to_cache();
    //from missq to dram

    // from dram to resp
}
void cache_interface::from_dramresp_to_resp()
{
    if (!dram_resp_queue.empty() and delay_resp_queue.size() < 64)
    {
        busy = true;
        auto addr = dram_resp_queue.front();
        dram_resp_queue.pop_front();
        m_cache.fill(addr);
        auto &v = addr_to_req[addr];
        for (auto &i : v)
        {
            delay_resp_queue.push_back(std::make_pair(componet::current_cycle, i));
        }
        addr_to_req.erase(addr);
    }
}
void cache_interface::from_miss_q_to_dram()
{
    if (!miss_queue.empty() and m_mem.WillAcceptTransaction(miss_queue.front(), false))
    {
        auto addr = miss_queue.front();
        miss_queue.pop_front();
        m_mem.AddTransaction(addr, false);
    }
}
void cache_interface::from_in_to_cache()
{
    if (!in_request_queue.empty() and miss_queue.size() < miss_size)
    {
        //TO-DO write access cache logic here!
        
    }
}