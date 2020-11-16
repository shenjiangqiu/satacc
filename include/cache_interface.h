#ifndef CACHE_INTERFACE_H
#define CACHE_INTERFACE_H
#include <deque>
#include "assign_wrap.h"
#include <sjqcache.h>
#include <iostream>
#include <map>
#include <vector>

#include <tuple>
#include "component.h"
#include <fmt/format.h>
#include <fmt/ranges.h>
#include <mem_req_interface.h>
#include <addr_utiles.h>
#include <enumerate.h>
#include <ramulator_wrapper.h>
//use template to avoid expesive divide calculate
class cache_stats
{
public:
    unsigned long long read_watcher_data_hit = 0;
    unsigned long long read_watcher_meta_hit = 0;
    unsigned long long read_watcher_value_hit = 0;
    unsigned long long read_clause_data_hit = 0;
    unsigned long long read_clause_value_hit = 0;
    unsigned long long write_watcher_list_hit = 0;
    unsigned long long write_clause_hit = 0;

    unsigned long long read_watcher_data_miss = 0;
    unsigned long long read_watcher_meta_miss = 0;
    unsigned long long read_watcher_value_miss = 0;
    unsigned long long read_clause_data_miss = 0;
    unsigned long long read_clause_value_miss = 0;
    unsigned long long write_watcher_list_miss = 0;
    unsigned long long write_clause_miss = 0;
    unsigned long long evict_write_hit = 0;
    unsigned long long evict_write_miss = 0;
    unsigned long long write_miss_read_hit = 0;
    unsigned long long write_miss_read_miss = 0;

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
        case AccessType::ReadWatcherMetaData:
            hit ? read_watcher_meta_hit++ : read_watcher_meta_miss++;
            break;
        case AccessType::WriteClause:
            hit ? write_clause_hit++ : write_clause_miss++;
            break;
        case AccessType::WriteWatcherList:
            hit ? write_watcher_list_hit++ : write_watcher_list_miss++;
            break;
        case AccessType::EvictWrite:
            hit ? evict_write_hit++ : evict_write_miss++;
            break;
        case AccessType::WriteMissRead:
            hit ? write_miss_read_hit++ : write_miss_read_miss++;
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
    ramulator_wrapper m_mem;
    unsigned n_partition;
    std::map<uint64_t, std::vector<req_ptr>> addr_to_req; //global,
    std::vector<bool> this_busy;
    std::vector<unsigned long long> busys;
    std::vector<unsigned long long> idles;
    std::vector<std::deque<std::pair<uint64_t, req_ptr>>> delay_resp_queues; //multiple queue

    //ture=read, false=write
    std::deque<std::pair<bool, uint64_t>> miss_queue; //read? addr

    std::deque<uint64_t> dram_resp_queue;
    uint64_t get_cache_addr(uint64_t addr);
    uint64_t from_cache_addr_to_real_addr(uint64_t cache_addr, int partition_id);

    //will consume the request in in_requests
    bool from_in_to_cache();
    bool from_miss_q_to_dram();
    bool from_dramresp_to_resp();
    bool from_delayresp_to_out();

    void read_call_back(uint64_t addr);

    void write_call_back(uint64_t);

    std::array<uint64_t, (int)AccessType::max> access_hist = {0};
    std::vector<std::array<uint64_t, (int)AccessType::max>> access_hists;
    bool is_ideal_dram;
    bool ideal_l3;
    unsigned num_ports;

    /* data */
public:
    unsigned get_partition_id(uint64_t addr);

    std::string get_line_trace() const override;
    std::string get_internal_size() const override;
    bool empty() const override;

    bool recieve_rdy(unsigned partition_id) const;
    cache_interface(unsigned int total_size, unsigned num_partion, bool ideal_dram, bool ideal_l3, unsigned num_ports, std::string mem_config_name,
                    uint64_t &t);
    cache_interface(int cache_set_assositive,
                    int cache_num_sets,
                    int cache_mshr_entries,
                    int cache_mshr_maxmerge,
                    unsigned num_partition,
                    bool,
                    bool,
                    unsigned,
                    std::string,
                    uint64_t &t);
    ~cache_interface();

    //interfaces
    std::vector<std::deque<req_ptr>> in_request_queues;
    std::vector<std::deque<req_ptr>> out_resp_queues;

protected:
    virtual bool do_cycle() override;
};

#endif