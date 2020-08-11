#include "cache_interface.h"
#include "component.h"
#include <functional>

uint64_t componet::current_cycle = 0;
cache_interface::cache_interface(int cache_set_assositive,
                                 int cache_num_sets,
                                 int cache_mshr_entries,
                                 int cache_mshr_maxmerge) : m_cache(cache_set_assositive, cache_num_sets, cache::lru, cache_mshr_entries, cache_mshr_maxmerge, "l3cache"),
                                                            m_mem(
                                                                "DDR4_4Gb_x16_2133_2.ini", "./",
                                                                [this](uint64_t addr) { read_call_back(addr); },
                                                                [this](uint64_t addr) { write_call_back(addr); })
{
    //m_mem("DDR4_4Gb_x16_2133_2.ini", "./", std::bind(read_call_back, std::placeholders::_1), std::bind(write_call_back, std::placeholders::_1));
}
cache_interface::~cache_interface() {}
void cache_interface::read_call_back(uint64_t addr)
{
    dram_resp_queue.push_back(addr);
}
void cache_interface::write_call_back(uint64_t addr)
{
    dram_resp_queue.push_back(addr);
}
bool cache_interface::cycle()
{
    bool busy = false;
    //current_cycle++;
    if (current_cycle % 16 == 0)
    {
        m_mem.ClockTick();
    }
    //from in to resp or missq
    busy |= from_delayresp_to_out();
    busy |= from_dramresp_to_resp();
    busy |= from_miss_q_to_dram();
    busy |= from_in_to_cache();

    if (busy)
        this->busy++;
    else
        this->idle++;
    return busy;
    //from missq to dram

    // from dram to resp
}

bool cache_interface::from_delayresp_to_out()
{
    bool busy = false;
    if (!delay_resp_queue.empty())
    {
        busy = true;
        if (current_cycle > delay_resp_queue.front().first + cache_delay)
        {
            //busy = true;
            auto &req = delay_resp_queue.front().second;
            out_resp_queue.push_back(req);
            delay_resp_queue.pop_front();
        }
    }
    return busy;
}
bool cache_interface::from_dramresp_to_resp()
{
    bool busy = false;

    if (!dram_resp_queue.empty() and delay_resp_queue.size() < 64)
    {
        busy = true;
        auto addr = dram_resp_queue.front();
        dram_resp_queue.pop_front();
        m_cache.fill(addr);
        assert(!addr_to_req.empty());
        auto &v = addr_to_req[addr];
        for (auto &i : v)
        {
            delay_resp_queue.push_back(std::make_pair(componet::current_cycle, i));
        }
        addr_to_req.erase(addr);
    }
    return busy;
}
bool cache_interface::from_miss_q_to_dram()
{
    bool busy = false;
    if (!miss_queue.empty() and m_mem.WillAcceptTransaction(miss_queue.front(), false))
    {
        busy = true;
        auto addr = miss_queue.front();
        miss_queue.pop_front();
        m_mem.AddTransaction(addr, false);
    }
    return busy;
}
bool cache_interface::from_in_to_cache()
{
    bool busy = false;

    if (!in_request_queue.empty() and miss_queue.size() < miss_size)
    {
        busy = true;
        //TO-DO write access cache logic here!
        //step one: get addr for this request.

        auto &req = in_request_queue.front();
        auto &as = req.as;
        auto type = req.type;
        auto watcherId = req.watcherId;
        auto clauseId = req.clauseId;
        uint64_t addr = 0;
        int cache_type = 0;

        switch (type)
        {
        case ReadType::ReadClauseData:
            /* code */

            addr = as->get_clause_addr(watcherId);
            addr += clauseId * 4;
            cache_type = 1;
            break;
        case ReadType::ReadClauseValue:
            addr = as->get_clause_detail(watcherId)[clauseId];
            cache_type = 1;
            break;
        case ReadType::ReadWatcher:
            addr = as->get_addr();
            addr += 4 * watcherId;
            break;
        case ReadType::WatcherReadValue:
            addr = as->get_block_addr(watcherId);

            break;

        default:
            break;
        }

        addr = addr & ((1ull << 32) - 1); //only cover 4gb memory
        auto cache_result = m_cache.try_access(addr, cache_type);
        auto block_addr = addr & ~((1 << 6) - 1);
        //auto cache_result = cache::hit;
        if (cache_result == cache::resfail)
        {
            return false;
        }

        if (cache_result == cache::miss && miss_queue.size() >= miss_size)
        {
            return false;
        }

        cache_result = m_cache.access(addr, cache_type);
        //cache_result = cache::hit;
        switch (cache_result)
        {
        case cache::hit:
            delay_resp_queue.push_back(std::make_pair(current_cycle, req));
            /* code */
            break;
        case cache::hit_res:

            assert(!addr_to_req[block_addr].empty()); //must not be empty
            addr_to_req[block_addr].push_back(req);

            break;
        case cache::miss:
            assert(addr_to_req[block_addr].empty()); //must be empty
            addr_to_req[block_addr].push_back(req);
            miss_queue.push_back(block_addr);

            break;
        default:
            break;
        }

        in_request_queue.pop_front();
    }
    return busy;
}