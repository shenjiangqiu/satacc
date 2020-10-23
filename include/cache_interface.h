#ifndef CACHE_INTERFACE_H
#define CACHE_INTERFACE_H
#include <deque>
#include "assign_wrap.h"
#include <sjqcache.h>
#include <iostream>
#include <map>
#include <vector>
#include "memory_system.h"
#include <tuple>
#include "component.h"
#include <fmt/format.h>
#include <fmt/ranges.h>
#include <mem_req_interface.h>
#include <addr_utiles.h>
#include <enumerate.h>
//use template to avoid expesive divide calculate
class cache_stats
{
public:
    unsigned long long read_watcher_data_hit = 0;
    unsigned long long read_watcher_value_hit = 0;
    unsigned long long read_clause_data_hit = 0;
    unsigned long long read_clause_value_hit = 0;
    unsigned long long write_watcher_list_hit = 0;
    unsigned long long write_clause_hit = 0;

    unsigned long long read_watcher_data_miss = 0;
    unsigned long long read_watcher_value_miss = 0;
    unsigned long long read_clause_data_miss = 0;
    unsigned long long read_clause_value_miss = 0;
    unsigned long long write_watcher_list_miss = 0;
    unsigned long long write_clause_miss = 0;

    void update(bool hit, const cache_interface_req &req)
    {
        switch (req.type)
        {
        case AccessType::ReadClauseData:
            /* code */
            hit ? read_clause_data_hit++ : read_clause_data_miss++;
            break;
        case AccessType::ReadClauseValue:
            hit ? read_clause_value_hit++ : read_clause_value_miss++;
            break;
        case AccessType::ReadWatcherValue:
            hit ? read_watcher_value_hit++ : read_watcher_value_miss++;
            break;
        case AccessType::ReadWatcherData:
            hit ? read_watcher_data_hit++ : read_watcher_data_miss++;
            break;
        case AccessType::writeClause:
            hit ? write_clause_hit++ : write_clause_miss++;
            break;
        case AccessType::writeWatcherList:
            hit ? write_watcher_list_hit++ : write_watcher_list_miss++;
            break;
        default:
            throw;
        }
    }
};
class cache_interface : public componet
{
    using req_ptr = std::unique_ptr<cache_interface_req>;
    using req_ptr_q = std::deque<req_ptr>;
    using req_ptr_q_vec = std::vector<req_ptr_q>;

private:
    cache_stats m_stats;
    unsigned cache_delay = 2;
    unsigned delay_q_size = 64;
    unsigned miss_size = 64;
    unsigned in_size = 64;
    std::vector<sjq::cache> m_caches;
    dramsim3::MemorySystem m_mem;
    unsigned n_partition;
    std::map<uint64_t, std::vector<req_ptr>> addr_to_req; //global,

    std::vector<std::deque<std::pair<uint64_t, req_ptr>>> delay_resp_queues; //multiple queue

    //ture=read, false=write
    std::deque<std::pair<bool, uint64_t>> miss_queue; //read? addr

    std::deque<uint64_t> dram_resp_queue;
    uint64_t get_cache_addr(uint64_t addr)
    {
        auto block_content = addr & ((1ull << 6) - 1);
        auto new_addr = addr >> get_log(n_partition);
        new_addr = addr & ~((1ull << 6) - 1);
        return new_addr + block_content;
    }
    uint64_t from_cache_addr_to_real_addr(uint64_t cache_addr, int partition_id)
    {
        auto block_content = cache_addr & ((1ull << 6) - 1);
        auto new_addr = cache_addr & ~((1ull << 6) - 1);
        new_addr = new_addr << get_log(n_partition);
        return new_addr + (partition_id << 6) + block_content;
    }
    //will consume the request in in_requests
    bool from_in_to_cache()
    {
        bool busy = false;
        for (auto &&[i, in_request_queue] : enumerate(in_request_queues))
            if (!in_request_queue.empty() and miss_queue.size() < miss_size)
            {
                busy = true;

                //step one: get addr for this request.

                auto &req = in_request_queue.front();
                auto &as = req->as;
                auto type = req->type;
                auto watcherId = req->watcherId;
                auto clauseId = req->clauseId;
                uint64_t addr = 0;
                auto cache_type = sjq::cache::read;
                access_hist[(int)type]++;

                switch (type)
                {
                case AccessType::ReadClauseData:
                    /* code */

                    addr = as->get_clause_addr(watcherId);
                    addr += clauseId * 4;
                    cache_type = sjq::cache::read;

                    break;
                case AccessType::ReadClauseValue:
                    addr = as->get_clause_detail(watcherId)[clauseId];
                    //cache_type = 1;
                    break;
                case AccessType::ReadWatcherData:
                    assert(as != nullptr);
                    addr = as->get_addr();
                    addr += 4 * watcherId;
                    break;
                case AccessType::ReadWatcherValue:
                    assert(as->get_watcher_size() != 0);
                    addr = as->get_block_addr(watcherId);

                    break;
                case AccessType::writeClause:
                    cache_type = sjq::cache::write;
                    addr = as->get_clause_addr(watcherId);
                    break;
                case AccessType::writeWatcherList:
                    //in_request_queue.pop_front();
                    cache_type = sjq::cache::write;
                    if (as->get_is_push_to_other(watcherId))
                        addr = as->get_pushed_watcher_list_tail_addr(watcherId);
                    else
                        addr = as->get_addr(); //current watcher list
                    //TODO need to change this
                    break;
                default:
                    throw;
                    break;
                }

                addr = addr & ((1ull << 32) - 1); //only cover 4gb memory
                //addr should remove the partition mask...
                //auto partition_bits = get_log(N);
                auto cache_addr = get_cache_addr(addr);

                auto cache_result = m_caches[i].try_access(cache_addr, cache_type);
                auto block_addr = addr & ~((1ull << 6ull) - 1ull);
                //auto cache_result = cache::hit;
                if (cache_result == sjq::cache::resfail)
                {
                    return false;
                }

                if (cache_result == sjq::cache::miss && miss_queue.size() >= miss_size)
                {
                    return false;
                }

                cache_result = m_caches[i].access(cache_addr, cache_type); //fix the cache access behavior, now can support write access
                m_stats.update(!(cache_result == sjq::cache::miss), *req);
                auto last_evicted = cache_result == sjq::cache::miss ? m_caches[i].get_last_evict() : 0;
                //here we need to deal with the write traffic,

                if (cache_type == sjq::cache::write)
                {
                    in_request_queue.pop_front();
                    if (cache_result == sjq::cache::miss)
                    {
                        if (last_evicted)
                        {
                            miss_queue.push_back({false, from_cache_addr_to_real_addr(last_evicted, i)});
                        }
                    }
                    return true;
                }
                //cache_result = sjq::cache::hit;
                switch (cache_result)
                {
                case sjq::cache::hit:

                    delay_resp_queues[i].push_back(std::make_pair(current_cycle, std::move(req)));
                    /* code */
                    break;
                case sjq::cache::hit_res:

                    assert(!addr_to_req[block_addr].empty()); //must not be empty
                    addr_to_req[block_addr].push_back(std::move(req));

                    break;
                case sjq::cache::miss:
                {
                    assert(addr_to_req[block_addr].empty()); //must be empty
                    addr_to_req[block_addr].push_back(std::move(req));
                    auto evict_addr = m_caches[i].get_last_evict();
                    if (evict_addr)
                        miss_queue.push_back({false, from_cache_addr_to_real_addr(evict_addr, i)});
                    miss_queue.push_back({true, block_addr}); //read request
                    //new we can add dram traffic of evicted addr.
                    break;
                }
                default:
                    break;
                }

                in_request_queue.pop_front();
            }
        return busy;
    }
    bool from_miss_q_to_dram()
    {
        bool busy = false;
        bool is_write = !miss_queue.front().first;
        if (!miss_queue.empty() and m_mem.WillAcceptTransaction(miss_queue.front().second, is_write))
        {
            busy = true;
            auto addr = miss_queue.front().second;
            miss_queue.pop_front();
            m_mem.AddTransaction(addr, is_write);
        }
        return busy;
    }
    bool from_dramresp_to_resp()
    {
        bool busy = false;

        if (!dram_resp_queue.empty())
        {
            //TODO delay response queue here
            //first get the partition
            auto addr = dram_resp_queue.front(); //this will be the blockaddr we send to dram

            auto partition_id = get_partition_id(addr);
            auto cache_addr = get_cache_addr(addr);
            auto &delay_resp_queue = delay_resp_queues[partition_id];
            if (delay_resp_queue.size() < delay_q_size)
            {
                busy = true;

                dram_resp_queue.pop_front();
                m_caches[partition_id].fill(cache_addr);
                assert(!addr_to_req[addr].empty());
                auto &v = addr_to_req[addr];
                for (auto &i : v)
                {
                    delay_resp_queue.push_back(std::make_pair(componet::current_cycle, std::move(i)));
                }
                addr_to_req.erase(addr);
            }
        }
        return busy;
    }
    bool from_delayresp_to_out()
    {
        bool busy = false;
        for (auto &&[i, delay_resp_queue] : enumerate(delay_resp_queues))
        {
            if (!delay_resp_queue.empty())
            {
                busy = true;
                if (current_cycle > delay_resp_queue.front().first + cache_delay)
                {
                    //busy = true;
                    auto &req = delay_resp_queue.front().second;
                    out_resp_queues[i].push_back(std::move(req));
                    delay_resp_queue.pop_front();
                }
            }
        }
        return busy;
    }

    void read_call_back(uint64_t addr)
    {
        dram_resp_queue.push_back(addr);
    }
    void write_call_back(uint64_t)
    {
        //igore the request
    }
    std::array<uint64_t, (int)AccessType::max> access_hist = {0};

    /* data */
public:
    unsigned get_partition_id(uint64_t addr)
    {
        return get_partition_id_by_addr(addr, n_partition);
    }
    std::string get_line_trace() const override
    {
        std::string ret;
        for (auto &m_cache : m_caches)
        {
            auto c = *m_cache.get_stats();
            ret += (fmt::format("\n{}:{}\n", "cache_interface", get_busy_percent()) +
                    fmt::format("c.num_hit {} ,c.num_hit_reserved {}  ,c.num_miss {} ,c.num_res_fail {} \n",
                                c.num_hit, c.num_hit_reserved, c.num_miss, c.num_res_fail) +
                    fmt::format("the_hist {}\n", access_hist));
        }

        ret += fmt::format("\nread_watcher_data_hit {}\nread_watcher_value_hit {}\nread_clause_data_hit {}\nread_clause_value_hit {}\nwrite_watcher_list_hit {}\nwrite_clause_hit {}\nread_watcher_data_miss {}\nread_watcher_value_miss {}\nread_clause_data_miss {}\nread_clause_value_miss {}\nwrite_watcher_list_miss {}\nwrite_clause_miss {}\n",
                           m_stats.read_watcher_data_hit,
                           m_stats.read_watcher_value_hit,
                           m_stats.read_clause_data_hit,
                           m_stats.read_clause_value_hit,
                           m_stats.write_watcher_list_hit,
                           m_stats.write_clause_hit,
                           m_stats.read_watcher_data_miss,
                           m_stats.read_watcher_value_miss,
                           m_stats.read_clause_data_miss,
                           m_stats.read_clause_value_miss,
                           m_stats.write_watcher_list_miss,
                           m_stats.write_clause_miss);
        return ret;
    }
    std::string get_internal_size() const override
    {
        std::string ret;
        for (auto i = 0u; i < n_partition; i++)
        {
            ret += fmt::format("name delay in out \n {} {} {} {}\n",
                               "cache_interface:",
                               delay_resp_queues[i].size(),
                               in_request_queues[i].size(),
                               out_resp_queues[i].size());
        }
        return ret + fmt::format("name addr_to_req missq dram_r  \n {} {} {} {}\n",
                                 "cache_interface:",
                                 addr_to_req.size(),
                                 miss_queue.size(),
                                 dram_resp_queue.size());
    }
    bool empty() const override
    {
        bool empty = true;
        for (auto i = 0u; i < n_partition; i++)
        {
            empty = empty and delay_resp_queues[i].empty() and in_request_queues[i].empty() and out_resp_queues[i].empty();
        }
        return empty and addr_to_req.empty() and miss_queue.empty() and dram_resp_queue.empty();
    }

    bool recieve_rdy(unsigned partition_id) const
    {
        return in_request_queues[partition_id].size() < in_size;
    }
    cache_interface(unsigned int total_size, unsigned num_partion,
                    uint64_t &t);
    cache_interface(int cache_set_assositive,
                    int cache_num_sets,
                    int cache_mshr_entries,
                    int cache_mshr_maxmerge,
                    unsigned num_partition,
                    uint64_t &t) : componet(t),
                                   m_caches(num_partition,
                                            sjq::cache(cache_set_assositive,
                                                       cache_num_sets / num_partition,
                                                       sjq::cache::lru,
                                                       cache_mshr_entries,
                                                       cache_mshr_maxmerge,
                                                       "l3cache")),
                                   m_mem(
                                       "DDR4_4Gb_x16_2133_2.ini", "./",
                                       [this](uint64_t addr) { read_call_back(addr); },
                                       [this](uint64_t addr) { write_call_back(addr); }),
                                   n_partition(num_partition),
                                   delay_resp_queues(num_partition),
                                   in_request_queues(num_partition),
                                   out_resp_queues(num_partition)

    {
        //m_mem("DDR4_4Gb_x16_2133_2.ini", "./", std::bind(read_call_back, std::placeholders::_1), std::bind(write_call_back, std::placeholders::_1));
    }
    ~cache_interface() {}

    //interfaces
    std::vector<std::deque<req_ptr>> in_request_queues;
    std::vector<std::deque<req_ptr>> out_resp_queues;

protected:
    virtual bool do_cycle() override
    {
        bool busy = false;
        //current_cycle++;
        m_mem.ClockTick();

        //from in to resp or missq
        busy |= from_delayresp_to_out();
        busy |= from_dramresp_to_resp();
        busy |= from_miss_q_to_dram();
        busy |= from_in_to_cache();

        return busy;
        //from missq to dram

        // from dram to resp
    }
};

#endif