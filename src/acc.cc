#include "acc.h"
acc::acc(unsigned t_num_watchers, unsigned t_num_clauses, uint64_t &tcurrent_cycle) : componet(tcurrent_cycle), num_watchers(t_num_watchers), num_clauses(t_num_clauses)
{
    // add the componets s
    m_cache_interface = new cache_interface(16, 1 << 16, 196, 4, current_cycle);
    m_componets.push_back(m_cache_interface);
    for (unsigned i = 0; i < num_watchers; i++)
    {
        auto new_watcher = new watcher(current_cycle);
        watchers.push_back(new_watcher);
        m_componets.push_back(new_watcher);
    }
    for (unsigned i = 0; i < num_clauses; i++)
    {
        auto new_clause = new clause(current_cycle);
        clauses.push_back(new_clause);
        m_componets.push_back(new_clause);
    }

    for (unsigned i = 0; i < num_watchers; i++)
    {
        //the pass for 1, watcher to clause, 2, wathcer and clause to memory. Do not include the internal cycle()
        clock_passes.push_back([i, this]() -> bool {
            bool busy = false;
            static int ii = 0; //current cycle's choice
            int watcher_id = i;
            int clause_id = watcher_id * (num_clauses / num_watchers) + ii;
            ii = (ii + 1) % (num_clauses / num_watchers);
            //send the request to clause unit
            if (!watchers.at(watcher_id)->out_send_queue.empty() and clauses.at(clause_id)->recieve_rdy())
            {
                busy = true;
                clauses[clause_id]->in_task_queue.push_back(watchers[watcher_id]->out_send_queue.front());
                watchers[watcher_id]->out_send_queue.pop_front();
            }

            //send the memory request to cache interfase

            if (!watchers[watcher_id]->out_memory_read_queue.empty() and m_cache_interface->recieve_rdy())
            {
                busy = true;
                m_cache_interface->in_request_queue.push_back(watchers[watcher_id]->out_memory_read_queue.front());
                m_cache_interface->in_request_queue.back().ComponentId = watcher_id;
                watchers[watcher_id]->out_memory_read_queue.pop_front();
            }
            //for each claue belong to the watcher
            for (unsigned c_n = 0; c_n < (num_clauses / num_watchers); c_n++)
            {
                if (!clauses[watcher_id * (num_clauses / num_watchers) + c_n]->out_memory_read_queue.empty() and m_cache_interface->recieve_rdy())
                {
                    busy = true;
                    m_cache_interface->in_request_queue.push_back(clauses[clause_id]->out_memory_read_queue.front());
                    m_cache_interface->in_request_queue.back().ComponentId = clause_id + num_watchers;

                    clauses[clause_id]->out_memory_read_queue.pop_front();
                }
            }
            return busy;
        });
    }
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
                if (clauses[clause_id]->recieve_mem_rdy())
                {
                    busy = true;

                    clauses[clause_id]->in_memory_resp_queue.push_back(m_cache_interface->out_resp_queue.front());
                    m_cache_interface->out_resp_queue.pop_front();
                }
            }
            else
            {
                //the case it's a watcher request
                int watcher_id = component_id;
                if (watchers[watcher_id]->recieve_mem_rdy())
                {
                    busy = true;
                    watchers[watcher_id]->in_memory_resp_queue.push_back(m_cache_interface->out_resp_queue.front());
                    m_cache_interface->out_resp_queue.pop_front();
                }
            }
        }
        return busy;
    });
    //

    //add the pass for from trail to watchers

    clock_passes.push_back([this]() -> bool {
        bool busy = false;
        static int ii = 0;
        int watcher_id = ii;

        if (!in_m_trail.empty() and watchers[watcher_id]->recieve_rdy())
        {
            busy = true;
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