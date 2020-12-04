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
    uint64_t get()
    {
        assert(!empty);
        empty = true;
        uint64_t total_cycle = 0;

        while (!in_reqs.empty())
        {
            auto &req = in_reqs.front();
            //auto current_watcher_list_q = std::queue<req_ptr>();
            //current_watcher_list_q.push(std::move(req));
            //in_reqs.pop();
            //1,read watcher list meta data,
            if (req->as->get_watcher_size() == 0)
            {
                in_reqs.pop();
                continue;
            }
            req->type = AccessType::ReadWatcherMetaData;
            req->watcherId = 0;
            m_cache_interface.in_request_queues[0].push_back(std::move(req));
            in_reqs.pop();

            while (m_cache_interface.out_resp_queues[0].empty())
            {
                assert(!m_cache_interface.empty());
                m_cache_interface.cycle();
                //std::cout<<m_cache_interface.get_internal_size()<<std::endl;
                total_cycle++;
            }

            auto out_req = std::move(m_cache_interface.out_resp_queues[0].front());
            m_cache_interface.out_resp_queues[0].pop_front();
            total_cycle++;
            //2, process the watchers
            while (out_req->watcherId != (unsigned)out_req->as->get_watcher_size())
            {
                //1,read watcher,write watcher
                auto new_req = copy_unit_ptr(out_req);
                new_req->type = AccessType::ReadWatcherData;
                m_cache_interface.in_request_queues[0].push_back(std::move(new_req));
                while (m_cache_interface.out_resp_queues[0].empty())
                {
                    m_cache_interface.cycle();
                    total_cycle++;
                }
                auto read_watcher_req = std::move(m_cache_interface.out_resp_queues[0].front());
                m_cache_interface.out_resp_queues[0].pop_front();
                total_cycle++;
                if (read_watcher_req->as->is_read_clause(out_req->watcherId))
                {
                    //read clause
                    //2,read clause. write clause
                    //calculate the clause address;
                    total_cycle++;
                    read_watcher_req->type = AccessType::ReadClauseData;
                    m_cache_interface.in_request_queues[0].push_back(std::move(read_watcher_req));
                    while (m_cache_interface.out_resp_queues[0].empty())
                    {
                        m_cache_interface.cycle();
                        total_cycle++;
                    }
                    auto read_clause_data_req = std::move(m_cache_interface.out_resp_queues[0].front());
                    m_cache_interface.out_resp_queues[0].pop_front();

                    //read value;
                    total_cycle++;

                    //process the clause;
                    total_cycle += 16;

                    //write back the watcher list;
                    if (read_clause_data_req->as->is_genereated(read_clause_data_req->watcherId))
                    {
                        auto ass = read_clause_data_req->as;
                        auto watcherId = read_clause_data_req->watcherId;
                        auto new_req = copy_unit_ptr(read_clause_data_req);

                        if (ass->get_generated(watcherId) != nullptr)
                        {
                            new_req->as = ass->get_generated(watcherId);
                            assert(new_req->as != nullptr);
                            new_req->watcherId = 0;
                            new_req->clauseId = 0;
                            new_req->type = AccessType::ReadWatcherMetaData;
                            assert(new_req);
                            //std::cout << fmt::format("in acc, {} generate {}\n", read_clause_data_req->as->get_value(), new_req->as->get_value());
                            in_reqs.push(std::move(new_req));
                        }
                    }
                    if (read_clause_data_req->as->get_is_push_to_other(read_clause_data_req->watcherId))
                    {
                        //write to the other watcher list
                        //read watcher list meta data
                        read_clause_data_req->type = AccessType::ReadOtherWatcherList;
                        m_cache_interface.in_request_queues[0].push_back(std::move(read_clause_data_req));
                        while (m_cache_interface.out_resp_queues[0].empty())
                        {
                            m_cache_interface.cycle();
                            total_cycle++;
                        }
                        auto finished_read_meta = std::move(m_cache_interface.out_resp_queues[0].front());
                        m_cache_interface.out_resp_queues[0].pop_front();

                        finished_read_meta->type = AccessType::WriteWatcherList;
                        m_cache_interface.in_request_queues[0].push_back(std::move(finished_read_meta));
                        m_cache_interface.cycle();
                        m_cache_interface.cycle();
                        total_cycle++;
                    }
                    else
                    {
                        total_cycle++;
                        read_clause_data_req->type = AccessType::WriteWatcherList;
                        m_cache_interface.in_request_queues[0].push_back(std::move(read_clause_data_req));
                        m_cache_interface.cycle();
                        m_cache_interface.cycle();
                        //write the the current watcher list
                    }

                    //3,write watcher
                }
                else
                {
                    //write back watcher list;
                    //the write back is ready, do nothing
                    total_cycle++;
                }

                out_req->watcherId++;
            }
        }
        return total_cycle;
        //do the simulation
    }

private:
    //sjq::cache m_l3_cache;
    cache_interface m_cache_interface;
    uint64_t cycle;
    bool empty;

    std::queue<req_ptr> in_reqs;
};

#endif