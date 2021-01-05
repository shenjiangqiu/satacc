#include "cache_interface.h"
#include "component.h"
#include <functional>

#include <enumerate.h>
#include <req_addr.h>

cache_interface::cache_interface(unsigned int total_size, unsigned num_partition, bool ideal_dram, bool ideal_l3,
                                 unsigned num_ports, std::string config_name,
                                 uint64_t &t) : cache_interface(16, total_size >> 10, 192, 4, num_partition, ideal_dram,
                                                                ideal_l3, num_ports, config_name, t) {
}

uint64_t cache_interface::get_cache_addr(uint64_t addr) {
    auto block_content = addr & ((1ull << 6) - 1);
    auto new_addr = addr >> get_log(n_partition);
    new_addr = addr & ~((1ull << 6) - 1);
    return new_addr + block_content;
}

uint64_t cache_interface::from_cache_addr_to_real_addr(uint64_t cache_addr, int partition_id) const {
    auto block_content = cache_addr & ((1ull << 6) - 1);
    auto new_addr = cache_addr & ~((1ull << 6) - 1);
    new_addr = new_addr << get_log(n_partition);
    return new_addr + (partition_id << 6) + block_content;
}

//will consume the request in in_requests
bool cache_interface::from_in_to_cache() {
    bool busy = false;
    for (auto &&[i, in_request_queue] : enumerate(in_request_queues)) {
        auto remain_ports = num_ports;
        while (!in_request_queue.empty() and miss_queue.size() < miss_size and remain_ports-- > 0) {
            busy = true;
            this_busy[i] = true;

            //step one: get addr for this request.

            auto &req = in_request_queue.front();
            if (ideal_l3) {
                if (req->type == AccessType::WriteClause or req->type == AccessType::WriteWatcherList or
                    req->type == AccessType::EvictWrite) {
                    in_request_queue.pop_front();
                } else {
                    delay_resp_queues[i].push_back({internal_cycle, std::move(req)});

                    in_request_queue.pop_front();
                }
                continue;
            }
            auto type = req->type;
            uint64_t addr = 0;
            auto cache_type = sjq::cache::read;
            access_hist[(int) type]++;
            access_hists[i][(int) type]++;

            addr = get_addr_by_req(req);
            if (req->type == AccessType::WriteClause or req->type == AccessType::WriteWatcherList or
                req->type == AccessType::EvictWrite) {
                cache_type = sjq::cache::write;
            }
            addr = addr & ((1ull << 32) - 1); //only cover 4gb memory
            //addr should remove the partition mask...
            //auto partition_bits = get_log(N);
            auto cache_addr = get_cache_addr(addr);

            auto cache_result = m_caches[i].try_access(cache_addr, cache_type);
            auto block_addr = addr & ~((1ull << 6ull) - 1ull);
            //auto cache_result = cache::hit;
            if (cache_result == sjq::cache::resfail) {
                continue;
            }

            if (cache_result == sjq::cache::miss && miss_queue.size() >= miss_size) {
                continue;
            }

            cache_result = m_caches[i].access(cache_addr, cache_type,
                                              (int) req->type); //fix the cache access behavior, now can support write access
            m_stats.update(cache_result != sjq::cache::miss, *req);
            //here we need to deal with the write traffic,
            if (cache_type == sjq::cache::write) {
                if (cache_result == sjq::cache::miss and m_caches[i].get_last_evict().get_tag() != 0 and
                    m_caches[i].get_last_evict().isDirty()) {
                    miss_queue.emplace_back(false,
                                            from_cache_addr_to_real_addr(m_caches[i].get_last_evict().get_tag() << 6,
                                                                         i));
                    cache_interface_req n_req = *req;
                    auto t_type = (AccessType) m_caches[i].get_last_evict().getMType();
                    std::cout << t_type << std::endl;
                    if (t_type == AccessType::ReadClauseData) {
                        t_type = AccessType::WriteClause;
                    } else if (t_type == AccessType::ReadWatcherData) {
                        t_type = AccessType::WriteWatcherList;
                    } else {
                        assert(t_type == AccessType::WriteWatcherList or t_type == AccessType::WriteClause);
                    }
                    n_req.type = t_type;
                    std::cout << t_type << std::endl;

                    m_stats.update(false, n_req);
                    //     std::cout << "send write:" << addr << std::endl;
                }
                in_request_queue.pop_front();

                continue;
            }
            //for the read access
            //cache_result = sjq::cache::hit;
            switch (cache_result) {
                case sjq::cache::hit:

                    delay_resp_queues[i].push_back(std::make_pair(internal_cycle, std::move(req)));
                    /* code */
                    break;
                case sjq::cache::hit_res:

                    assert(!addr_to_req[block_addr].empty()); //must not be empty
                    addr_to_req[block_addr].push_back(std::move(req));

                    break;
                case sjq::cache::miss: {
                    assert(addr_to_req[block_addr].empty()); //must be empty
                    addr_to_req[block_addr].push_back(std::move(req));
                    auto evict_entry = m_caches[i].get_last_evict();
                    if (evict_entry.get_tag() != 0 and evict_entry.isDirty()) {
                        miss_queue.push_back({false, from_cache_addr_to_real_addr(evict_entry.get_tag() << 6, i)});
                        // std::cout << "send write:" << addr << std::endl;
                        cache_interface_req n_req = *req;
                        auto t_type = (AccessType) m_caches[i].get_last_evict().getMType();
                        std::cout << t_type << std::endl;
                        if (t_type == AccessType::ReadClauseData) {
                            t_type = AccessType::WriteClause;
                        } else if (t_type == AccessType::ReadWatcherData) {
                            t_type = AccessType::WriteWatcherList;
                        } else {
                            assert(t_type == AccessType::WriteWatcherList or t_type == AccessType::WriteClause);
                        }
                        n_req.type = t_type;
                        std::cout << t_type << std::endl;

                        m_stats.update(false, n_req);
                    }
                    miss_queue.push_back({true, block_addr}); //read request
                    //std::cout << fmt::format("read from {}: {}", i, block_addr) << std::endl;

                    //new we can add dram traffic of evicted addr.
                    break;
                }
                default:
                    break;
            }

            in_request_queue.pop_front();
        }
    }
    return busy;
}

bool cache_interface::from_miss_q_to_dram() {
    if (miss_queue.empty()) {
        return false;
    }
    bool busy = false;
    bool is_read = miss_queue.front().first;
    if (is_ideal_dram) {
        dram_resp_queue.push_back(miss_queue.front().second);
        miss_queue.pop_front();
        busy = true;
        return busy;
    }
    //the tuple: first means is read, second is the address.
    if (!miss_queue.empty()) {
        busy = true;
        auto addr = miss_queue.front().second;
        // std::cout << "from missq:" << addr << std::endl;
        miss_queue.pop_front();
        m_mem.send(addr, is_read);
    }
    return busy;
}

bool cache_interface::from_dramresp_to_resp() {
    bool busy = false;

    if (!dram_resp_queue.empty()) {
        //TODO delay response queue here
        //first get the partition
        auto addr = dram_resp_queue.front(); //this will be the blockaddr we send to dram

        auto partition_id = get_partition_id(addr);
        auto cache_addr = get_cache_addr(addr);
        auto &delay_resp_queue = delay_resp_queues[partition_id];
        if (delay_resp_queue.size() < delay_q_size) {
            busy = true;

            dram_resp_queue.pop_front();
            m_caches[partition_id].fill(cache_addr);
            assert(!addr_to_req[addr].empty());
            auto &v = addr_to_req[addr];
            for (auto &i : v) {
                delay_resp_queue.push_back(std::make_pair(internal_cycle, std::move(i)));
            }
            addr_to_req.erase(addr);
        }
    }
    return busy;
}

bool cache_interface::from_delayresp_to_out() {
    bool busy = false;
    for (auto &&[i, delay_resp_queue] : enumerate(delay_resp_queues)) {
        if (!delay_resp_queue.empty()) {
            this_busy[i] = true;
            busy = true;
            if (internal_cycle > delay_resp_queue.front().first + cache_delay) {
                //busy = true;
                auto &req = delay_resp_queue.front().second;
                out_resp_queues[i].push_back(std::move(req));
                delay_resp_queue.pop_front();
            }
        }
    }
    return busy;
}

void cache_interface::read_call_back(uint64_t addr) {
    dram_resp_queue.push_back(addr);
}

void cache_interface::write_call_back(uint64_t) {
    //igore the request
}

unsigned cache_interface::get_partition_id(uint64_t addr) {
    return get_partition_id_by_addr(addr, n_partition);
}

std::string cache_interface::get_line_trace() const {
    std::string ret;
    ret += fmt::format("\n{}:{}\n", "cache_interface", get_busy_percent()) +
           "\nReadWatcherData,ReadWatcherValue,ReadWatcherMetaData,ReadClauseData,ReadClauseValue,WriteWatcherList,WriteClause,EvictWrite,WriteMissRead " +
           fmt::format(" the_hist", access_hist);
    for (int i = 0; i < n_partition; i++) {
        auto c = *m_caches[i].get_stats();
        ret += (fmt::format("\n{}:{} {} {}\n", "cache_interface", busys[i], idles[i],
                            (double) busys[i] / (double) (busys[i] + idles[i])) +
                fmt::format("c.num_hit {} ,c.num_hit_reserved {}  ,c.num_miss {} ,c.num_res_fail {} \n",
                            c.num_hit, c.num_hit_reserved, c.num_miss, c.num_res_fail) +
                fmt::format("the_hist {}\n", access_hists[i]));
    }

    ret += fmt::format(
            "\nread_watcher_data_hit {}\nread_watcher_value_hit {}\nread_clause_data_hit {}\nread_clause_value_hit {}\nwrite_watcher_list_hit {}\nwrite_clause_hit {}\nread_watcher_data_miss {}\nread_watcher_value_miss {}\nread_clause_data_miss {}\nread_clause_value_miss {}\nwrite_watcher_list_miss {}\nwrite_clause_miss {}\n",
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

std::string cache_interface::get_internal_size() const {
    std::string ret;
    for (auto i = 0u; i < n_partition; i++) {
        ret += fmt::format("name delay in out \n {} {} {} {}\n",
                           "cache_interface:",
                           delay_resp_queues[i].size(),
                           in_request_queues[i].size(),
                           out_resp_queues[i].size());
    }

    ret += fmt::format("name addr_to_req missq dram_r  \n {} {} {} {}\n",
                       "cache_interface:",
                       addr_to_req.size(),
                       miss_queue.size(),
                       dram_resp_queue.size());
    ret += m_mem.get_internal_size();
    return ret;
}

bool cache_interface::empty() const {
    bool empty = true;
    for (auto i = 0u; i < n_partition; i++) {
        empty = empty and delay_resp_queues[i].empty() and in_request_queues[i].empty() and out_resp_queues[i].empty();
    }
    return empty and addr_to_req.empty() and miss_queue.empty() and dram_resp_queue.empty() and m_mem.empty();
}

bool cache_interface::recieve_rdy(unsigned partition_id) const {
    return in_request_queues[partition_id].size() < in_size;
}

cache_interface::cache_interface(int cache_set_assositive,
                                 int cache_num_sets,
                                 int cache_mshr_entries,
                                 int cache_mshr_maxmerge,
                                 unsigned num_partition,
                                 bool tis_ideal_memory,
                                 bool ideal_l3,
                                 unsigned num_ports,
                                 std::string config_name,
                                 uint64_t &t) : componet(t),
                                                m_caches(num_partition,
                                                         sjq::cache(cache_set_assositive,
                                                                    cache_num_sets / num_partition,
                                                                    sjq::cache::lru,
                                                                    cache_mshr_entries,
                                                                    cache_mshr_maxmerge,
                                                                    "l3cache")),
                                                m_mem(config_name, 64, t),
                                                n_partition(num_partition),
                                                this_busy(num_partition, false),
                                                busys(num_partition, 0),
                                                idles(num_partition, 0),
                                                delay_resp_queues(num_partition),
                                                access_hists(n_partition, {0}),
                                                is_ideal_dram(tis_ideal_memory),
                                                ideal_l3(ideal_l3),
                                                num_ports(num_ports),
                                                in_request_queues(num_partition),
                                                out_resp_queues(num_partition) {

    //m_mem("DDR4_4Gb_x16_2133_2.ini", "./", std::bind(read_call_back, std::placeholders::_1), std::bind(write_call_back, std::placeholders::_1));
}

cache_interface::~cache_interface() {}

bool cache_interface::do_cycle() {
    internal_cycle++;
    bool busy = false;
    std::fill(this_busy.begin(), this_busy.end(), false); //fill all to not busy;
    busy |= m_mem.cycle();
    while (m_mem.return_avaliable()) {
        auto addr = m_mem.pop();
        dram_resp_queue.push_back(addr);
    }
    //from in to resp or missq
    busy |= from_delayresp_to_out();
    busy |= from_dramresp_to_resp();
    busy |= from_miss_q_to_dram();
    busy |= from_in_to_cache();
    for (int i = 0; i < n_partition; i++) {
        if (this_busy[i]) {
            busys[i]++;
        } else {
            idles[i]++;
        }
    }

    return busy;
    //from missq to dram

    // from dram to resp
}
