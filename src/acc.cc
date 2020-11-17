#include "acc.h"
#include <mem_req_interface.h>

#include <req_addr.h>
#include <addr_utiles.h>
#include <new_intersim_wrapper.h>
#include <memreq_info.h>
#include <fstream>
#include <sstream>

namespace sjq
{
    bool inside_busy;
} // namespace sjq

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

bool acc::from_watcher_icnt_out()
{

    for (auto i = 0; i < num_clauses; i++)
    {
        if (!watcher_icnt->out_reqs[i].empty())
        {
            auto &req = watcher_icnt->out_reqs[i].front();
            clauses[i]->in_task_queue.push_back(std::move(req));
            watcher_icnt->out_reqs[i].pop_front();
        }
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
            if (!watchers.at(watcher_id)->out_send_queue.empty() and watcher_icnt->in_reqs[watcher_id].size() < 64)
            {
                busy = true;
                watcher_icnt->in_reqs[watcher_id].push_back(std::move(watchers[watcher_id]->out_send_queue.front()));
                watchers[watcher_id]->out_send_queue.pop_front();
            }

            //send the memory request to cache interfase //it's direct to l3 cache
            if (!watchers[watcher_id]->out_memory_read_queue.empty())
            {
                if (watchers[watcher_id]->out_memory_read_queue.front()->type == AccessType::ReadWatcherData)
                {
                    auto source = watcher_id;
                    if (m_icnt->has_buffer(MEM_L2, source))
                    {
                        busy = true;
                        assert(watchers[watcher_id]->out_memory_read_queue.front()->as != nullptr);
                        auto &req = watchers[watcher_id]->out_memory_read_queue.front();

                        assert(req->type == AccessType::ReadWatcherData);
                        //error here
                        //m_cache_interface->in_request_queue.push_back(std::move(req));
                        //m_cache_interface->in_request_queue.back()->ComponentId = watcher_id;
                        //now push to icnt
                        req->ComponentId = watcher_id;
#ifdef SJQ_ICNT_DEBUG
                        std::cout << "from watcher to icnt: " << i << std::endl;
                        std::cout << *req << std::endl;

#endif
                        m_icnt->in_reqs[source].push_back(std::move(req));

                        watchers[watcher_id]->out_memory_read_queue.pop_front();
                    }
                }
                //send private cache to cache//to private cache!!
                else if (watchers[watcher_id]->out_memory_read_queue.front()->type == AccessType::ReadWatcherMetaData)
                {
                    auto source = watcher_id;
                    if (m_icnt->has_buffer(MEM_L2, source))
                    {
                        busy = true;
                        assert(watchers[watcher_id]->out_memory_read_queue.front()->as != nullptr);
                        auto &req = watchers[watcher_id]->out_memory_read_queue.front();

                        assert(req->type == AccessType::ReadWatcherMetaData);
                        //error here
                        //m_cache_interface->in_request_queue.push_back(std::move(req));
                        //m_cache_interface->in_request_queue.back()->ComponentId = watcher_id;
                        //now push to icnt
                        req->ComponentId = watcher_id;
#ifdef SJQ_ICNT_DEBUG
                        std::cout << "from watcher to icnt: " << i << std::endl;
                        std::cout << *req << std::endl;

#endif
                        m_icnt->in_reqs[source].push_back(std::move(req));

                        watchers[watcher_id]->out_memory_read_queue.pop_front();
                    }
                }
                else
                {
                    assert(watchers[watcher_id]->out_memory_read_queue.front()->type == AccessType::ReadWatcherValue);
                    if (!watchers[watcher_id]->out_memory_read_queue.empty() and m_private_caches[watcher_id]->recieve_rdy())
                    {
                        busy = true;
                        assert(watchers[watcher_id]->out_memory_read_queue.front()->as != nullptr);
                        auto &req = watchers[watcher_id]->out_memory_read_queue.front();
                        req->ComponentId = watcher_id;
                        assert(req->type == AccessType::ReadWatcherValue);
#ifdef SJQ_ICNT_DEBUG
                        std::cout << "from watcher to private cache: " << i << std::endl;
                        std::cout << *req << std::endl;

#endif
                        m_private_caches[watcher_id]->in_request.push_back(std::move(req));

                        watchers[watcher_id]->out_memory_read_queue.pop_front();
                    }
                }
            }
            //send private cache to l3 cache
            if (!m_private_caches[watcher_id]->out_miss_queue.empty())
            {
                auto &req = m_private_caches[watcher_id]->out_miss_queue.front();
                if (m_icnt->has_buffer(MEM_L2, watcher_id))
                {
                    busy = true;
                    //req->ComponentId = watcher_id;
#ifdef SJQ_ICNT_DEBUG
                    std::cout << "from private cache to icnt: " << i << std::endl;
                    std::cout << *req << std::endl;

#endif
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
                    if (clauses[clauseId]->out_memory_read_queue.front()->type == AccessType::ReadClauseData)
                    {
                        auto source = clauseId / (n_c / n_w);
                        if (m_icnt->has_buffer(MEM_L2, source))
                        {
                            busy = true;
                            assert(clauses[clauseId]->out_memory_read_queue.front()->as != nullptr);
                            auto &req = clauses[clauseId]->out_memory_read_queue.front();
                            req->ComponentId = clauseId + num_watchers;
#ifdef SJQ_ICNT_DEBUG
                            std::cout << fmt::format("from clause to icnt:{} {}", clauseId, source) << std::endl;
                            //std::cout << "from clause to icnt: " << i << std::endl;
                            std::cout << *req << std::endl;

#endif
                            m_icnt->in_reqs[source].push_back(std::move(req));
                            clauses[clauseId]->out_memory_read_queue.pop_front();
                        }
                    }
                    else
                    {
                        //push value request to private cache
                        auto &req = clauses[clauseId]->out_memory_read_queue.front();
                        assert(req->type == AccessType::ReadClauseValue);
                        auto watcherId = clauseId / (num_clauses / num_watchers);
                        if (m_private_caches[watcherId]->recieve_rdy())
                        {
                            busy = true;
                            assert(req->as != nullptr);
                            req->ComponentId = clauseId + num_watchers;
#ifdef SJQ_ICNT_DEBUG
                            //std::cout << "from clause to private cache" << std::endl;
                            std::cout << fmt::format("from clause to private cache:{} {}", clauseId, watcherId) << std::endl;

                            std::cout << *req << std::endl;

#endif
                            m_private_caches[watcherId]->in_request.push_back(std::move(clauses[clauseId]->out_memory_read_queue.front()));

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
        for (auto i = 0u; i < num_partition; i++)
        {
            if (!m_cache_interface->out_resp_queues[i].empty())
            {
                auto &req = m_cache_interface->out_resp_queues[i].front();
                assert(i == get_partition_id_by_addr(get_addr_by_req(req), num_partition));
                m_icnt->in_resps[i].push_back(std::move(req));
                m_cache_interface->out_resp_queues[i].pop_front();
                /*
                //this code if for icnt to watcher and clause
                if (component_id >= num_watchers)
                {
                    //the case it's a clause request

                    int clause_id = component_id - num_watchers;

                    if (req->type == AccessType::ReadClauseData)
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

                        assert(m_cache_interface->out_resp_queue.front()->type == AccessType::ReadClauseValue);
                        int watcherId = clause_id / (num_clauses / num_watchers);
                        m_private_caches[watcherId]->in_resp.push_back(std::move(m_cache_interface->out_resp_queue.front()));
                        m_cache_interface->out_resp_queue.pop_front();
                    }
                }
                else
                {
                    //the case it's a watcher request
                    int watcher_id = component_id;
                    if (m_cache_interface->out_resp_queue.front()->type == AccessType::ReadWatcherData)
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
                        assert(m_cache_interface->out_resp_queue.front()->type == AccessType::ReadWatcherValue);
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
                if (req->type == AccessType::ReadClauseValue)
                {
                    assert(req->ComponentId >= num_watchers);
                    auto clauseId = req->ComponentId - num_watchers;
                    assert(clauseId / (num_clauses / num_watchers) == watcherId);
#ifdef SJQ_ICNT_DEBUG
                    std::cout << fmt::format("from private cache {} to clause {}", i, clauseId) << std::endl;
                    std::cout << *req << std::endl;
#endif
                    clauses[clauseId]->in_memory_resp_queue.push_back(std::move(req));
                    m_private_caches[watcherId]->out_send_q.pop_front();
                }
                else
                {
                    assert(req->type == AccessType::ReadWatcherValue);
                    assert(req->ComponentId == watcherId);
//std::cout << "watcher:" << watcherId << "push response from private cache" << std::endl;
#ifdef SJQ_ICNT_DEBUG
                    std::cout << fmt::format("from private cache {} to watcher {}", i, watcherId) << std::endl;
                    std::cout << *req << std::endl;
#endif
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
        if (enable_sequential and !std::all_of(m_componets.begin(), m_componets.end(), [](auto &c) { return c->empty(); }))
        {
            return false;
        }
        bool busy = false;
        static int ii = 0;
        int watcher_id = ii;

        if (!in_m_trail.empty() and watchers[watcher_id]->recieve_rdy())
        {
            busy = true;
            assert(in_m_trail.front()->as != nullptr);
            watchers[watcher_id]->in_task_queue.push_back(std::move(in_m_trail.front()));
            sjq::inside_busy = true;
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
                assert(req->ComponentId >= num_watchers);
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
            int remain_port = multi_l3cache_port;

            //this for loop: for each watcher unit
            while (!m_icnt->out_resps[i].empty() and watcher_unit->recieve_mem_rdy() and remain_port-- > 0)
            {

                //this if: this is from icnt to watchers or clauses or private cache
                busy = true;
                auto &req = m_icnt->out_resps[i].front(); //get the owner;
                if (req->ComponentId < num_watchers)
                {
                    //bug here
                    //need to send to private cache if it's value accesss
                    if (req->type == AccessType::ReadWatcherValue)
                    {
#ifdef SJQ_ICNT_DEBUG
                        std::cout << "req out from icnt to private cache,it's read watcher value: " << i << std::endl;
                        std::cout << *req << std::endl;
#endif
                        m_private_caches[i]->in_resp.push_back(std::move(req));
                    }
                    else if (req->type == AccessType::ReadWatcherMetaData)
                    {
                        watcher_unit->in_memory_resp_queue.push_back(std::move(req));
                    }
                    else
                    {
#ifdef SJQ_ICNT_DEBUG
                        std::cout << "req out from icnt to watcher unit,it's read watcher data " << i << std::endl;
                        std::cout << *req << std::endl;
#endif
                        assert(req->type == AccessType::ReadWatcherData);
                        watcher_unit->in_memory_resp_queue.push_back(std::move(req));
                    }
                }
                else
                {
                    //it's clause access
                    if (req->type == AccessType::ReadClauseValue)
                    {
#ifdef SJQ_ICNT_DEBUG
                        std::cout << "req out from icnt to clause unit,it's read clause value " << i << std::endl;
                        std::cout << *req << std::endl;
#endif
                        m_private_caches[i]->in_resp.push_back(std::move(req));
                    }
                    else
                    {
                        assert(req->type == AccessType::ReadClauseData);
                        assert(req->ComponentId >= num_watchers && req->ComponentId < num_watchers + num_clauses);
                        auto clause_id = (req->ComponentId - num_watchers);
                        assert(clause_id / (num_clauses / num_watchers) == i);
#ifdef SJQ_ICNT_DEBUG
                        std::cout << "req out from icnt to clause unit,it's read clause data " << i << std::endl;
                        std::cout << *req << std::endl;

#endif
                        clauses[clause_id]->in_memory_resp_queue.push_back(std::move(req));
                    }
                }
                m_icnt->out_resps[i].pop_front();
            }
        }
        for (auto i = 0u; i < num_partition; i++)
        {
            int remain_port = multi_l3cache_port;
            while (!m_icnt->out_reqs[i].empty() and m_cache_interface->recieve_rdy(i) and remain_port-- > 0)
            {

                auto &req = m_icnt->out_reqs[i].front();
                busy = true;
#ifdef SJQ_ICNT_DEBUG
                std::cout << "req out from icnt into cache interface " << i << std::endl;
                std::cout << *req << std::endl;
#endif
                m_cache_interface->in_request_queues[i].push_back(std::move(req));

                m_icnt->out_reqs[i].pop_front();
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

void acc::parse_file()
{
    const std::map<std::string, std::set<std::string>> all_settings = {
        {"n_watchers", {}},
        {"n_clauses", {}},
        {"mems", {}},
        {"icnt", {"mesh", "ideal", "ring"}},
        {"seq", {"true", "false"}},
        {"ideal_memory", {"true", "false"}},
        {"ideal_l3cache", {"true", "false"}},
        {"multi_port", {}},
        {"dram_config", {"ALDRAM-config.cfg", "DDR4-config.cfg", "GDDR5-config.cfg", "LPDDR3-config.cfg", "PCM-config.cfg", "STTMRAM-config.cfg", "WideIO2-config.cfg", "DDR3-config.cfg", "DSARP-config.cfg", "HBM-config.cfg", "LPDDR4-config.cfg", "SALP-config.cfg", "TLDRAM-config.cfg", "WideIO-config.cfg"}},
        {"watcher_icnt", {"mesh", "ideal", "ring"}}};

    const std::string input_file_name = "satacc_config.txt";
    std::ifstream in(input_file_name);
    std::string icnt_type;
    std::map<std::string, std::string> acc_config;
    std::string line;
    //read the config
    while (std::getline(in, line))
    {
        if (line[0] == '#' or line[0] == ',' or line[0] == ' ')
        {
            //this line is comment
            continue;
        }
        std::istringstream in_line(line);
        std::string key;
        if (std::getline(in_line, key, '='))
        {

            std::string value;
            if (std::getline(in_line, value))
            {
                acc_config.insert({key, value});
            }
        }
    }
    for (auto &&config : acc_config)
    {
        std::cout << fmt::format("{},{}\n", config.first, config.second);
    }
    //fisrt, valide the count
    if (!acc_config.size() == all_settings.size())
    {
        throw std::runtime_error("the number of settings are not equal");
    }
    for (auto &&config : acc_config)
    {
        if (!all_settings.count(config.first))
        {
            throw std::runtime_error("no such setting!");
        }
        if (!all_settings.at(config.first).empty() and !all_settings.at(config.first).count(config.second))
        {
            throw std::runtime_error("no such value!");
        }
    }

    num_watchers = std::stoul(acc_config["n_watchers"]);

    num_clauses = std::stoul(acc_config["n_clauses"]);

    //parse the number of mem partition
    unsigned num_mem;

    num_mem = std::stoul(acc_config["mems"]);

    num_partition = num_mem;
    //parse the icnt type

    auto icnt = acc_config["icnt"];
    if (icnt == "mesh")
    {
        m_icnt = new icnt_mesh(current_cycle,
                               num_watchers, num_mem, num_clauses, 3, 1, 0, 64, 3);
    }
    else if (icnt == "ring")
    {
        m_icnt = new icnt_ring(current_cycle,
                               num_watchers, num_mem, num_clauses, 3, 1, 0, 64, 3);
    }
    else if (icnt == "ideal")
    {
        m_icnt = new icnt_ideal(current_cycle, num_watchers, num_mem, num_clauses);
    }
    else
    {
        m_icnt = new icnt_mesh(current_cycle,
                               num_watchers, num_mem, num_clauses, 3, 1, 0, 64, 3);
    }
    enable_sequential = acc_config["seq"] == "true" ? true : false;
    ideal_memory = acc_config["ideal_memory"] == "true" ? true : false;
    ideal_l3cache = acc_config["ideal_l3cache"] == "true" ? true : false;

    auto number = std::stoul(acc_config["multi_port"]);
    if (number > 1)
    {
        multi_l3cache_port = number;
    }
    else
    {
        multi_l3cache_port = 1;
    }

    dram_config_file = acc_config["dram_config"];
    auto watcher_icnt_s = acc_config["watcher_icnt"];
    if (watcher_icnt_s == "mesh")
    {
        watcher_icnt = new icnt_mesh(current_cycle,
                                     num_watchers, num_clauses, num_clauses, 3, 1, 0, 64, 3, true);
    }
    else if (watcher_icnt_s == "ring")
    {
        watcher_icnt = new icnt_ring(current_cycle,
                                     num_watchers, num_clauses, num_clauses, 3, 1, 0, 64, 3, true);
    }
    else if (watcher_icnt_s == "ideal")
    {
        watcher_icnt = new icnt_ideal(current_cycle, num_watchers, num_clauses, num_clauses, true);
    }
    else
    {
        throw;
    }
}

acc::acc(unsigned t_num_watchers,
         unsigned t_num_clauses,
         unsigned private_cache_size,
         unsigned l3_cache_size,
         uint64_t &tcurrent_cycle) : componet(tcurrent_cycle),
                                     private_cache_size(private_cache_size),
                                     num_watchers(t_num_watchers),
                                     num_clauses(t_num_clauses)

{

    //read the configure file. will set num_watcher and num_clause again. so the previouse value will be override!
    parse_file();
    sjq::inside_busy = false;

    // add the componets s

    m_cache_interface = new cache_interface(l3_cache_size, num_partition, ideal_memory, ideal_l3cache, multi_l3cache_port, dram_config_file, current_cycle);
    m_componets.push_back(m_cache_interface);
    m_watcher_write_unit = new watcher_list_write_unit(current_cycle);
    m_clause_write_unit = new clause_writer(current_cycle);
    m_componets.push_back(m_watcher_write_unit);
    m_componets.push_back(m_clause_write_unit);
    //m_icnt = new icnt(tcurrent_cycle, t_num_watchers, 8, t_num_clauses);
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