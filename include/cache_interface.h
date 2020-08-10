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
enum class ReadType
{
    ReadWatcher,
    WatcherReadValue,
    ReadClauseData,
    ReadClauseValue
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
    default:
        break;
    }
    return os;
}

struct cache_interface_req
{
    cache_interface_req(ReadType t,
                        int w,
                        int cl,
                        int co,
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
    int watcherId;
    int clauseId;
    int ComponentId;
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
    int delay_q_size = 64;
    int miss_size = 64;
    cache m_cache;
    dramsim3::MemorySystem m_mem;

    std::map<uint32_t, std::vector<cache_interface_req>> addr_to_req;

    std::deque<std::pair<uint32_t, cache_interface_req>> delay_resp_queue;
    std::deque<uint32_t> miss_queue;

    std::deque<uint32_t> dram_resp_queue;

    void cycle() override;
    void from_in_to_cache();
    void from_miss_q_to_dram();
    void from_dramresp_to_resp();

    void read_call_back(uint64_t addr);
    void write_call_back(uint64_t addr);
    /* data */
public:
    cache_interface(/* args */);
    ~cache_interface();

    //interfaces
    std::deque<cache_interface_req> in_request_queue;
    std::deque<cache_interface_req> out_resp_queue;
};

#endif