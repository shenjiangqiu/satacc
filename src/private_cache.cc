#include "private_cache.h"

private_cache::private_cache(int asso, int total_size, uint64_t &tcurrent_cycle) : componet(tcurrent_cycle), m_cache(asso, total_size)
{
}

bool private_cache::cycle()
{
    bool busy = false;
    busy |= from_in_to_out();
    busy |= from_resp_to_send();
    if (busy)
        this->busy++;
    else
        idle++;
    return busy;
}

bool private_cache::from_in_to_out()
{
    bool busy = false;
    //from in to out;
    if (!in_request.empty() and out_send_q.size() < out_size)
    {
        auto &req = in_request.front();
        auto as = req.as;
        auto watcherId = req.watcherId;
        auto clauseId = req.clauseId;
        int cache_type = 0;
        uint64_t addr = 0;
        switch (req.type)
        {
        case ReadType::WatcherReadValue:
            /* code */
            assert(as->get_watcher_size() != 0);
            addr = as->get_block_addr(watcherId);

            break;
        case ReadType::ReadClauseValue:
            addr = as->get_clause_detail(watcherId)[clauseId];
            cache_type = 1;
            break;
        default:
            throw;
            break;
        }
        addr = addr & ((1ull << 32) - 1); //only cover 4gb memory
        auto block_addr = addr & ~((1 << 6) - 1);
        auto result = m_cache.try_access(block_addr, cache_type);
        if (result == cache::resfail)
        {
            busy = false;
        }
        else
        {
            busy = true;
            result = m_cache.access(block_addr, cache_type);
            switch (result)
            {
            case cache::hit:
                out_send_q.push_back(req);
                /* code */
                break;
            case cache::hit_res:
                assert(addr_to_req.find(block_addr) != addr_to_req.end());
                addr_to_req[block_addr].push_back(req);
                break;
            case cache::miss:
                out_miss_queue.push_back(req);
                assert(addr_to_req.find(block_addr) == addr_to_req.end());
                addr_to_req[block_addr].push_back(req);
                break;
            default:
                throw;
                break;
            }
            in_request.pop_front();
        }
    }

    return busy;
}

bool private_cache::from_resp_to_send()
{
    bool busy = false;

    if (!in_resp.empty() and out_send_q.size() < out_size)
    {
        busy = true;

        auto &req = in_resp.front();
        auto as = req.as;
        auto watcherId = req.watcherId;
        auto clauseId = req.clauseId;
        //int cache_type = 0;
        uint64_t addr = 0;
        switch (req.type)
        {
        case ReadType::WatcherReadValue:
            /* code */
            assert(as->get_watcher_size() != 0);
            addr = as->get_block_addr(watcherId);

            break;
        case ReadType::ReadClauseValue:
            addr = as->get_clause_detail(watcherId)[clauseId];
            //cache_type = 1;
            break;
        default:
            throw;
            break;
        }
        addr = addr & ((1ull << 32) - 1); //only cover 4gb memory
        //auto result = m_cache.try_access(addr, cache_type);
        auto block_addr = addr & ~((1 << 6) - 1);

        assert(addr_to_req.find(block_addr) != addr_to_req.end());
        m_cache.fill(block_addr);
        auto &v = addr_to_req[block_addr];
        for (auto &i : v)
        {
            out_send_q.push_back(i);
        }

        addr_to_req.erase(block_addr);

        in_resp.pop_front();
    }

    return busy;
}