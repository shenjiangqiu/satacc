#include "acc.h"
#include <mem_req_interface.h>
#include <icnt_wrapper.h>
#include <req_addr.h>
#include <addr_utiles.h>
void acc::init_watcher_and_clause()
{
    //init the components
    for (unsigned i = 0; i < num_watchers; i++)
    {
        auto new_watcher = new watcher(current_cycle);
        watchers.push_back(new_watcher);
        m_componets.push_back(new_watcher);

        auto m_private_cache = new private_cache(8, private_cache_size, current_cycle);
        m_componets.push_back(m_private_cache);
        m_private_caches.push_back(m_private_cache);
    }
    for (unsigned i = 0; i < num_clauses; i++)
    {
        auto new_clause = new clause(current_cycle);
        clauses.push_back(new_clause);
        m_componets.push_back(new_clause);
    }
}

void acc::add_hook_from_watcher_out_actions()
{
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
                clauses[clause_id]->in_task_queue.push_back(std::move(watchers[watcher_id]->out_send_queue.front()));
                watchers[watcher_id]->out_send_queue.pop_front();
            }

            //send the memory request to cache interfase //it's direct to l3 cache
            if (!watchers[watcher_id]->out_memory_read_queue.empty())
            {
                if (watchers[watcher_id]->out_memory_read_queue.front()->type == ReadType::ReadWatcher)
                {
                    auto source = watcher_id;
                    if (::icnt_has_buffer(source, 64))
                    {
                        busy = true;
                        assert(watchers[watcher_id]->out_memory_read_queue.front()->as != nullptr);
                        auto &req = watchers[watcher_id]->out_memory_read_queue.front();

                        assert(req->type == ReadType::ReadWatcher);
                        //error here
                        //m_cache_interface->in_request_queue.push_back(std::move(req));
                        //m_cache_interface->in_request_queue.back()->ComponentId = watcher_id;
                        //now push to icnt
                        req->ComponentId = watcher_id;
                        m_icnt->in_reqs[source].push_back(std::move(req));

                        watchers[watcher_id]->out_memory_read_queue.pop_front();
                    }
                }
                //send private cache to cache//to private cache!!
                else
                {
                    assert(watchers[watcher_id]->out_memory_read_queue.front()->type == ReadType::WatcherReadValue);
                    if (!watchers[watcher_id]->out_memory_read_queue.empty() and m_private_caches[watcher_id]->recieve_rdy())
                    {
                        busy = true;
                        assert(watchers[watcher_id]->out_memory_read_queue.front()->as != nullptr);
                        auto &req = watchers[watcher_id]->out_memory_read_queue.front();

                        assert(req->type == ReadType::WatcherReadValue);

                        m_private_caches[watcher_id]->in_request.push_back(std::move(req));
                        m_private_caches[watcher_id]->in_request.back()->ComponentId = watcher_id;

                        watchers[watcher_id]->out_memory_read_queue.pop_front();
                    }
                }
            }
            //send private cache to l3 cache
            if (!m_private_caches[watcher_id]->out_miss_queue.empty())
            {
                auto &req = m_private_caches[watcher_id]->out_miss_queue.front();
                if (icnt_has_buffer(watcher_id, 64))
                {
                    busy = true;
                    m_icnt->in_reqs[watcher_id].push_back(std::move(req));
                    m_private_caches[watcher_id]->out_miss_queue.pop_front();
                }
            }
            return busy;
        });
    }
}
void acc::add_hook_from_clause_to_mem()
{
    //send clause mem request
    for (unsigned i = 0; i < num_clauses; i++)
    {
        int clauseId = i;
        unsigned n_w = num_watchers;
        unsigned n_c = num_clauses;
        clock_passes.push_back(
            [clauseId, n_w, n_c, this]() {
                bool busy = false;
                if (!clauses[clauseId]->out_memory_read_queue.empty())
                {
                    if (clauses[clauseId]->out_memory_read_queue.front()->type == ReadType::ReadClauseData)
                    {
                        auto source = clauseId / (n_c / n_w);
                        if (icnt_has_buffer(source, 16))
                        {
                            busy = true;
                            assert(clauses[clauseId]->out_memory_read_queue.front()->as != nullptr);
                            auto &req = clauses[clauseId]->out_memory_read_queue.front();
                            req->ComponentId = clauseId + num_watchers;
                            m_icnt->in_reqs[source].push_back(std::move(req));
                            clauses[clauseId]->out_memory_read_queue.pop_front();
                        }
                    }
                    else
                    {
                        //push value request to private cache
                        assert(clauses[clauseId]->out_memory_read_queue.front()->type == ReadType::ReadClauseValue);
                        auto watcherId = clauseId / (num_clauses / num_watchers);
                        if (m_private_caches[watcherId]->recieve_rdy())
                        {
                            busy = true;
                            assert(clauses[clauseId]->out_memory_read_queue.front()->as != nullptr);
                            m_private_caches[watcherId]->in_request.push_back(std::move(clauses[clauseId]->out_memory_read_queue.front()));
                            m_private_caches[watcherId]->in_request.back()->ComponentId = clauseId + num_watchers;

                            clauses[clauseId]->out_memory_read_queue.pop_front();
                        }
                    }
                }

                return busy;
            });
    } //end for clause
}
void acc::add_hook_from_cache_to_clause_and_watchers()
{

    //add pass to send cache response to clauses and watchers
    clock_passes.push_back([this]() {
        bool busy = false;
        for (auto i = 0u; i < 8u; i++)
        {
            if (!m_cache_interface->out_resp_queues[i].empty())
            {
                auto &req = m_cache_interface->out_resp_queues[i].front();
                m_icnt->in_resps[i].push_back(std::move(req));
                m_cache_interface->out_resp_queues[i].pop_front();
                /*
                //this code if for icnt to watcher and clause
                if (component_id >= num_watchers)
                {
                    //the case it's a clause request

                    int clause_id = component_id - num_watchers;

                    if (req->type == ReadType::ReadClauseData)
                    { //send to cleause

                        if (clauses[clause_id]->recieve_mem_rdy())
                        {
                            busy = true;

                            clauses[clause_id]->in_memory_resp_queue.push_back(std::move(m_cache_interface->out_resp_queue.front()));
                            m_cache_interface->out_resp_queue.pop_front();
                        }
                    }
                    else
                    {
                        //send to private cache

                        assert(m_cache_interface->out_resp_queue.front()->type == ReadType::ReadClauseValue);
                        int watcherId = clause_id / (num_clauses / num_watchers);
                        m_private_caches[watcherId]->in_resp.push_back(std::move(m_cache_interface->out_resp_queue.front()));
                        m_cache_interface->out_resp_queue.pop_front();
                    }
                }
                else
                {
                    //the case it's a watcher request
                    int watcher_id = component_id;
                    if (m_cache_interface->out_resp_queue.front()->type == ReadType::ReadWatcher)
                    {
                        //it's a watcher data request , send to watcher
                        if (watchers[watcher_id]->recieve_mem_rdy())
                        {
                            busy = true;
                            watchers[watcher_id]->in_memory_resp_queue.push_back(std::move(m_cache_interface->out_resp_queue.front()));
                            m_cache_interface->out_resp_queue.pop_front();
                        }
                    }
                    else
                    {
                        assert(m_cache_interface->out_resp_queue.front()->type == ReadType::WatcherReadValue);
                        //int watcherId = clause_id / (num_clauses / num_watchers);
                        m_private_caches[watcher_id]->in_resp.push_back(std::move(m_cache_interface->out_resp_queue.front()));
                        m_cache_interface->out_resp_queue.pop_front();
                    }
                }
                */
            }
        }
        return busy;
    });
}
void acc::add_hook_from_private_cache()
{
    //from private cache to watchers and clauses.
    for (unsigned i = 0; i < num_watchers; i++)
    {
        clock_passes.push_back([i, this]() {
            bool busy = false;
            unsigned watcherId = i;
            if (!m_private_caches[watcherId]->out_send_q.empty())
            {
                busy = true;
                auto &req = m_private_caches[watcherId]->out_send_q.front();
                if (req->type == ReadType::ReadClauseValue)
                {
                    assert(req->ComponentId >= num_watchers);
                    auto clauseId = req->ComponentId - num_watchers;
                    assert(clauseId / (num_clauses / num_watchers) == watcherId);

                    clauses[clauseId]->in_memory_resp_queue.push_back(std::move(req));
                    m_private_caches[watcherId]->out_send_q.pop_front();
                }
                else
                {
                    assert(req->type == ReadType::WatcherReadValue);
                    assert(req->ComponentId == watcherId);
                    //std::cout << "watcher:" << watcherId << "push response from private cache" << std::endl;
                    watchers[watcherId]->in_memory_resp_queue.push_back(std::move(req));
                    m_private_caches[watcherId]->out_send_q.pop_front();
                }
            }
            return busy;
        });
    }
}
void acc::add_hook_from_trail_to_watcher()
{
    //add the pass for from trail to watchers

    clock_passes.push_back([this]() -> bool {
        bool busy = false;
        static int ii = 0;
        int watcher_id = ii;

        if (!in_m_trail.empty() and watchers[watcher_id]->recieve_rdy())
        {
            busy = true;
            assert(in_m_trail.front()->as != nullptr);
            watchers[watcher_id]->in_task_queue.push_back(std::move(in_m_trail.front()));
            in_m_trail.pop_front();
        }
        //change to another watcher now
        ii = (ii + 1) % num_watchers;
        return busy;
    });
}
void acc::add_hook_from_clause_to_trail()
{
    // add the pass for from clauses to trail
    for (unsigned i = 0; i < num_clauses; i++)
    {
        clock_passes.push_back([i, this]() {
            bool busy = false;
            int clause_id = i;
            if (!clauses[clause_id]->out_queue.empty())
            {
                busy = true;
                assert(clauses[clause_id]->out_queue.front()->as != nullptr);
                in_m_trail.push_back(std::move(clauses[clause_id]->out_queue.front()));
                clauses[clause_id]->out_queue.pop_front();
            }
            return busy;
        });
    }
}
void acc::add_hook_from_watcher_to_writeuite()
{

    //from watcher to watcher write_unit
    for (unsigned i = 0; i < num_watchers; i++)
    {
        clock_passes.push_back([i, this]() {
            bool busy = false;
            if (!watchers[i]->out_write_watcher_list_queue.empty())
            {
                busy = true;
                auto &req = watchers[i]->out_write_watcher_list_queue.front();
                req->ComponentId = i;
                m_watcher_write_unit->in_request.push_back(std::move(req));
                watchers[i]->out_write_watcher_list_queue.pop_front();
            }

            return busy;
        });
    }
}
void acc::add_hook_from_clause_to_writeuint()
{

    //from clause to clause write unit
    for (unsigned i = 0; i < num_clauses; i++)
    {
        clock_passes.push_back([i, this]() {
            bool busy = false;
            if (!clauses[i]->out_clause_write_queue.empty())
            {
                busy = true;
                auto &req = clauses[i]->out_clause_write_queue.front();
                req->ComponentId = i + num_watchers;
                m_clause_write_unit->in.push_back(std::move(req));
                clauses[i]->out_clause_write_queue.pop_front();
            }

            return busy;
        });
    }
}

void acc::add_hook_from_clause_write_unit_to_cache()
{
    //TODO from writer to cache
    clock_passes.push_back(
        [this]() {
            bool busy = false;
            if (!m_clause_write_unit->out.empty())
            {

                busy = true;
                auto &req = m_clause_write_unit->out.front();
                auto source = (req->ComponentId - num_watchers) / (num_clauses / num_watchers);
                m_icnt->in_reqs[source].push_back(std::move(req));

                //m_cache_interface->in_request_queue.push_back(std::move(req));
                m_clause_write_unit->out.pop_front();
            }
            return busy;
        });
}
void acc::add_hook_from_watcher_write_unit_to_cache()
{

#if __cplusplus >= 202002L
    //good
    clock_passes.push_back([=, this]() {
#else
    //from watceher write unit to l3 cache
    clock_passes.push_back([=]() {
#endif
        bool busy = false;
        if (!m_watcher_write_unit->out_mem_requst.empty())
        {
            auto &req = m_watcher_write_unit->out_mem_requst.front();
            auto source = req->ComponentId;
            m_icnt->in_reqs[source].push_back(std::move(req));
            m_watcher_write_unit->out_mem_requst.pop_front();
            busy = true;
        }
        return busy;
    });
}

void acc::add_hook_from_icnt_to_other()
{
    clock_passes.push_back([this]() {
        auto busy = false;
        for (auto &&[i, watcher_unit] : enumerate(watchers))
        {
            if (auto req_p = (cache_interface_req *)icnt_pop(i))
            {
                busy = true;
                auto req = std::unique_ptr<cache_interface_req>(req_p); //get the owner;
                if (req->ComponentId < num_watchers)
                {
                    watcher_unit->in_memory_resp_queue.push_back(std::move(req));
                }
                else
                {
                    assert(req->ComponentId >= num_watchers && req->ComponentId < num_watchers + num_clauses);
                    auto clause_id = (req->ComponentId - num_watchers) / (num_clauses / num_watchers);
                    clauses[clause_id]->in_memory_resp_queue.push_back(std::move(req));
                }
            }
        }
        for (auto i = 0u; i < 8; i++)
        {
            if (auto req_p = (cache_interface_req *)icnt_pop(i + num_watchers))
            {
                busy = true;
                auto req = std::unique_ptr<cache_interface_req>(req_p);
                m_cache_interface->in_request_queues[i].push_back(std::move(req));
            }
        }
        return busy;
    });
}
acc::acc(unsigned t_num_watchers,
         unsigned t_num_clauses,
         uint64_t &t_current_cycle) : acc(t_num_watchers, t_num_clauses,
                                          1 << 10, 16 << 20,
                                          t_current_cycle) {}
acc::acc(unsigned t_num_watchers,
         unsigned t_num_clauses,
         unsigned private_cache_size,
         unsigned l3_cache_size,
         uint64_t &tcurrent_cycle) : componet(tcurrent_cycle),
                                     private_cache_size(private_cache_size),
                                     num_watchers(t_num_watchers),
                                     num_clauses(t_num_clauses)

{
    //TODO too large in this fuction!!
    // add the componets s
    m_cache_interface = new cache_interface(l3_cache_size, current_cycle);
    m_componets.push_back(m_cache_interface);
    m_watcher_write_unit = new watcher_list_write_unit(current_cycle);
    m_clause_write_unit = new clause_writer(current_cycle);
    m_componets.push_back(m_watcher_write_unit);
    m_componets.push_back(m_clause_write_unit);
    m_icnt = new icnt(tcurrent_cycle, t_num_watchers, 8, t_num_clauses);
    m_componets.push_back(m_icnt);

    init_watcher_and_clause();

    add_hook_from_watcher_out_actions();
    add_hook_from_clause_to_mem();
    add_hook_from_cache_to_clause_and_watchers();
    add_hook_from_private_cache();
    add_hook_from_trail_to_watcher();
    add_hook_from_clause_to_trail();
    add_hook_from_watcher_to_writeuite();
    add_hook_from_clause_to_writeuint();
    add_hook_from_clause_write_unit_to_cache();
    add_hook_from_watcher_write_unit_to_cache();

    //new

    add_hook_from_icnt_to_other();
}

acc::~acc()
{
    for (auto &comp : m_componets)
    {
        delete comp;
    }
}

bool acc::do_cycle()
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
    return busy;
}