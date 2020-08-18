#include "acc.h"
acc::acc(unsigned t_num_watchers,
         unsigned t_num_clauses,
         uint64_t &tcurrent_cycle) : componet(tcurrent_cycle),
                                     num_watchers(t_num_watchers),
                                     num_clauses(t_num_clauses)
{
    // add the componets s
    m_cache_interface = new cache_interface(16, 1 << 16, 196, 4, current_cycle);
    m_componets.push_back(m_cache_interface);
    for (unsigned i = 0; i < num_watchers; i++)
    {
        auto new_watcher = new watcher(current_cycle);
        watchers.push_back(new_watcher);
        m_componets.push_back(new_watcher);

        auto m_private_cache = new private_cache(8, 1 << 10, current_cycle);
        m_componets.push_back(m_private_cache);
        m_private_caches.push_back(m_private_cache);
    }
    for (unsigned i = 0; i < num_clauses; i++)
    {
        auto new_clause = new clause(current_cycle);
        clauses.push_back(new_clause);
        m_componets.push_back(new_clause);
    }

    //handle from watchers to clauses, and from watchers to memory
    for (unsigned i = 0; i < num_watchers; i++)
    {
        //the pass for 1, watcher to clause, 2, wathcer and clause to memory. Do not include the internal cycle()
        clock_passes.push_back([i, this]() -> bool {
            bool busy = false;
            //static int ii = 0; //current cycle's choice
            int watcher_id = i;
            int total_c_p_w = num_clauses / num_watchers;
            int clause_id = watcher_id * total_c_p_w + watchers[watcher_id]->next_c(total_c_p_w);
            //bug here , ii will be shared across all the lambda!!!!!
            //ii = (ii + 1) % (num_clauses / num_watchers);
            //send the request to clause unit
            if (!watchers.at(watcher_id)->out_send_queue.empty() and clauses.at(clause_id)->recieve_rdy())
            {
                busy = true;
                clauses[clause_id]->in_task_queue.push_back(watchers[watcher_id]->out_send_queue.front());
                watchers[watcher_id]->out_send_queue.pop_front();
            }

            //send the memory request to cache interfase //it's direct to l3 cache
            if (!watchers[watcher_id]->out_memory_read_queue.empty())
            {
                if (watchers[watcher_id]->out_memory_read_queue.front().type == ReadType::ReadWatcher)
                {
                    if (m_cache_interface->recieve_rdy())
                    {
                        busy = true;
                        assert(watchers[watcher_id]->out_memory_read_queue.front().as != nullptr);
                        const auto &req = watchers[watcher_id]->out_memory_read_queue.front();

                        assert(req.type == ReadType::ReadWatcher);
                        m_cache_interface->in_request_queue.push_back(req);
                        m_cache_interface->in_request_queue.back().ComponentId = watcher_id;

                        watchers[watcher_id]->out_memory_read_queue.pop_front();
                    }
                }
                //send private cache to cache//to private cache!!
                else
                {
                    assert(watchers[watcher_id]->out_memory_read_queue.front().type == ReadType::WatcherReadValue);
                    if (!watchers[watcher_id]->out_memory_read_queue.empty() and m_private_caches[watcher_id]->recieve_rdy())
                    {
                        busy = true;
                        assert(watchers[watcher_id]->out_memory_read_queue.front().as != nullptr);
                        const auto &req = watchers[watcher_id]->out_memory_read_queue.front();

                        assert(req.type == ReadType::WatcherReadValue);
                        m_private_caches[watcher_id]->in_request.push_back(req);
                        m_private_caches[watcher_id]->in_request.back().ComponentId = watcher_id;

                        watchers[watcher_id]->out_memory_read_queue.pop_front();
                    }
                }
            }
            //send private cache to l3 cache
            if (!m_private_caches[watcher_id]->out_miss_queue.empty() and m_cache_interface->recieve_rdy())
            {
                busy = true;
                m_cache_interface->in_request_queue.push_back(m_private_caches[watcher_id]->out_miss_queue.front());
                m_private_caches[watcher_id]->out_miss_queue.pop_front();
            }
            return busy;
        });
    }

    //send clause mem request
    for (unsigned i = 0; i < num_clauses; i++)
    {
        int clauseId = i;

        clock_passes.push_back(
            [clauseId, this]() {
                bool busy = false;
                if (!clauses[clauseId]->out_memory_read_queue.empty())
                {
                    if (clauses[clauseId]->out_memory_read_queue.front().type == ReadType::ReadClauseData)
                    {
                        if (m_cache_interface->recieve_rdy())
                        {
                            busy = true;
                            assert(clauses[clauseId]->out_memory_read_queue.front().as != nullptr);
                            m_cache_interface->in_request_queue.push_back(clauses[clauseId]->out_memory_read_queue.front());
                            m_cache_interface->in_request_queue.back().ComponentId = clauseId + num_watchers;

                            clauses[clauseId]->out_memory_read_queue.pop_front();
                        }
                    }
                    else
                    {
                        //push value request to private cache
                        assert(clauses[clauseId]->out_memory_read_queue.front().type == ReadType::ReadClauseValue);
                        auto watcherId = clauseId / (num_clauses / num_watchers);
                        if (m_private_caches[watcherId]->recieve_rdy())
                        {
                            busy = true;
                            assert(clauses[clauseId]->out_memory_read_queue.front().as != nullptr);
                            m_private_caches[watcherId]->in_request.push_back(clauses[clauseId]->out_memory_read_queue.front());
                            m_private_caches[watcherId]->in_request.back().ComponentId = clauseId + num_watchers;

                            clauses[clauseId]->out_memory_read_queue.pop_front();
                        }
                    }
                }

                return busy;
            });
    } //end for clause

    //add pass to send cache response to clauses and watchers
    clock_passes.push_back([this]() {
        bool busy = false;

        if (!m_cache_interface->out_resp_queue.empty())
        {
            auto component_id = m_cache_interface->out_resp_queue.front().ComponentId;
            if (component_id >= num_watchers)
            {
                //the case it's a clause request

                int clause_id = component_id - num_watchers;

                if (m_cache_interface->out_resp_queue.front().type == ReadType::ReadClauseData)
                { //send to cleause
                    if (clauses[clause_id]->recieve_mem_rdy())
                    {
                        busy = true;

                        clauses[clause_id]->in_memory_resp_queue.push_back(m_cache_interface->out_resp_queue.front());
                        m_cache_interface->out_resp_queue.pop_front();
                    }
                }
                else
                {
                    //send to private cache

                    assert(m_cache_interface->out_resp_queue.front().type == ReadType::ReadClauseValue);
                    int watcherId = clause_id / (num_clauses / num_watchers);
                    m_private_caches[watcherId]->in_resp.push_back(m_cache_interface->out_resp_queue.front());
                    m_cache_interface->out_resp_queue.pop_front();
                }
            }
            else
            {
                //the case it's a watcher request
                int watcher_id = component_id;
                if (m_cache_interface->out_resp_queue.front().type == ReadType::ReadWatcher)
                {
                    //it's a watcher data request , send to watcher
                    if (watchers[watcher_id]->recieve_mem_rdy())
                    {
                        busy = true;
                        watchers[watcher_id]->in_memory_resp_queue.push_back(m_cache_interface->out_resp_queue.front());
                        m_cache_interface->out_resp_queue.pop_front();
                    }
                }
                else
                {
                    assert(m_cache_interface->out_resp_queue.front().type == ReadType::WatcherReadValue);
                    //int watcherId = clause_id / (num_clauses / num_watchers);
                    m_private_caches[watcher_id]->in_resp.push_back(m_cache_interface->out_resp_queue.front());
                    m_cache_interface->out_resp_queue.pop_front();
                }
            }
        }
        return busy;
    });
    //from private cache to watchers and clauses.
    for (unsigned i = 0; i < num_watchers; i++)
    {
        clock_passes.push_back([i, this]() {
            bool busy = false;
            unsigned watcherId = i;
            if (!m_private_caches[watcherId]->out_send_q.empty())
            {
                busy = true;
                auto req = m_private_caches[watcherId]->out_send_q.front();
                if (req.type == ReadType::ReadClauseValue)
                {
                    assert(req.ComponentId >= num_watchers);
                    auto clauseId = req.ComponentId - num_watchers;
                    assert(clauseId / (num_clauses / num_watchers) == watcherId);

                    clauses[clauseId]->in_memory_resp_queue.push_back(req);
                    m_private_caches[watcherId]->out_send_q.pop_front();
                }
                else
                {
                    assert(req.type == ReadType::WatcherReadValue);
                    assert(req.ComponentId == watcherId);
                    //std::cout << "watcher:" << watcherId << "push response from private cache" << std::endl;
                    watchers[watcherId]->in_memory_resp_queue.push_back(req);
                    m_private_caches[watcherId]->out_send_q.pop_front();
                }
            }
            return busy;
        });
    }

    //add the pass for from trail to watchers

    clock_passes.push_back([this]() -> bool {
        bool busy = false;
        static int ii = 0;
        int watcher_id = ii;

        if (!in_m_trail.empty() and watchers[watcher_id]->recieve_rdy())
        {
            busy = true;
            assert(in_m_trail.front().as != nullptr);
            watchers[watcher_id]->in_task_queue.push_back(in_m_trail.front());
            in_m_trail.pop_front();
        }
        //change to another watcher now
        ii = (ii + 1) % num_watchers;
        return busy;
    });

    // add the pass for from clauses to trail
    for (unsigned i = 0; i < num_clauses; i++)
    {
        clock_passes.push_back([i, this]() {
            bool busy = false;
            int clause_id = i;
            if (!clauses[clause_id]->out_queue.empty())
            {
                busy = true;
                assert(clauses[clause_id]->out_queue.front().as != nullptr);
                in_m_trail.push_back(clauses[clause_id]->out_queue.front());
                clauses[clause_id]->out_queue.pop_front();
            }
            return busy;
        });
    }
}

acc::~acc()
{
    for (auto &comp : m_componets)
    {
        delete comp;
    }
}

bool acc::cycle()
{
    bool busy = false;
    //data transimission for all the components
    //clock_passes setup at acc()
    for (auto &f : clock_passes)
    {
        busy |= f();
    }
    //clock all the internal components
    for (auto &c : m_componets)
    {
        busy |= c->cycle();
    }
    if (busy)
        this->busy++;
    else
        this->idle++;
    return busy;
}