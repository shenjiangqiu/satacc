#ifndef CACHE_INTERFACE_H
#define CACHE_INTERFACE_H
#include <deque>
#include "assign_wrap.h"
#include <cache.h>
#include <iostream>
#include <map>
#include <vector>
#include "memory_system.h"
#include <tuple>
#include "component.h"
#include <fmt/format.h>
#include <fmt/ranges.h>
enum class ReadType
{
    ReadWatcher,
    WatcherReadValue,
    ReadClauseData,
    ReadClauseValue,
    writeWatcherList,
    writeClause,
    max
};

template <typename OSTYPE>
OSTYPE &operator<<(OSTYPE &os, const ReadType &req)
{
    switch (req)
    {
    case ReadType::ReadWatcher:
        os << "ReadWatcher";
        break;
    case ReadType::WatcherReadValue:
        os << "WatcherReadValue";
        break;
    case ReadType::ReadClauseData:
        os << "ReadClauseData";
        break;
    case ReadType::ReadClauseValue:
        os << "ReadClauseValue";
        break;
    case ReadType::writeWatcherList:
        os << "writeWatcherList";
        break;
    case ReadType::writeClause:
        os << "writeClause";
        break;
    default:
        throw;
        break;
    }
    return os;
}

struct cache_interface_req
{
    cache_interface_req(ReadType t,
                        unsigned w,
                        unsigned cl,
                        unsigned co,
                        assign_wrap *a) : type(t),
                                          watcherId(w),
                                          clauseId(cl),
                                          ComponentId(co),
                                          as(a) {}
    bool operator==(const cache_interface_req &other) const
    {
        return type == other.type and
               watcherId == other.watcherId and
               clauseId == other.clauseId and
               ComponentId == other.ComponentId and
               as == other.as;
    }
    ReadType type;
    unsigned watcherId;
    unsigned clauseId;
    unsigned ComponentId;
    assign_wrap *as;
    /* data */
};
template <typename OSTYPE>
OSTYPE &operator<<(OSTYPE &os, const cache_interface_req &req)
{
    os << req.type << "," << req.watcherId << "," << req.clauseId << "," << req.ComponentId << "," << req.as << ".";
    return os;
}

class cache_interface : public componet
{
private:
    unsigned cache_delay = 2;
    unsigned delay_q_size = 64;
    unsigned miss_size = 64;
    unsigned in_size = 64;
    cache m_cache;
    dramsim3::MemorySystem m_mem;

    std::map<uint64_t, std::vector<cache_interface_req>> addr_to_req;

    std::deque<std::pair<uint64_t, cache_interface_req>> delay_resp_queue;
    std::deque<uint64_t> miss_queue;

    std::deque<uint64_t> dram_resp_queue;

    bool from_in_to_cache();
    bool from_miss_q_to_dram();
    bool from_dramresp_to_resp();
    bool from_delayresp_to_out();

    void read_call_back(uint64_t addr);
    void write_call_back(uint64_t addr);
    std::array<uint64_t, (int)ReadType::max> access_hist = {0};
    /* data */
public:
    std::string get_line_trace() const override
    {
        auto c = *m_cache.get_stats();
        return fmt::format("{}:{}\n", "cache_interface", get_busy_percent()) +
               fmt::format("c.num_hit {} ,c.num_hit_reserved {}  ,c.num_miss {} ,c.num_res_fail {} \n",
                           c.num_hit, c.num_hit_reserved, c.num_miss, c.num_res_fail) +
               fmt::format("the_hist {}", access_hist);
    }
    std::string get_internal_size() const override
    {
        return fmt::format("name delay addr_to_req missq dram_r in_req out_rep\n{} {} {} {} {} {} {}",
                           "cache_interface:",
                           delay_resp_queue.size(),
                           addr_to_req.size(),
                           miss_queue.size(),
                           dram_resp_queue.size(),
                           in_request_queue.size(),
                           out_resp_queue.size());
    }
    bool empty() const override
    {
        return delay_resp_queue.empty() and addr_to_req.empty() and miss_queue.empty() and dram_resp_queue.empty() and
               in_request_queue.empty() and out_resp_queue.empty();
    }
    bool cycle() override;
    bool recieve_rdy() const
    {
        return in_request_queue.size() < in_size;
    }
    cache_interface(int cache_set_assositive, int cache_num_sets, int cache_mshr_entries, int cache_mshr_maxmerge, uint64_t &t);
    cache_interface(unsigned total_size, uint64_t &t);
    ~cache_interface();

    //interfaces
    std::deque<cache_interface_req> in_request_queue;
    std::deque<cache_interface_req> out_resp_queue;
};

#endif