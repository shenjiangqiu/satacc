#ifndef ACC_H
#define ACC_H
#include <vector>
#include <deque>

#include "watcher.h"
#include "clause.h"
#include "cache_interface.h"

#include "assign_wrap.h"

#include "component.h"
#include <functional>
#include <algorithm>
#include <string>
#include <fmt/format.h>
#include "private_cache.h"
#include "watcher_list_write_unit.h"
#include "clause_writer.h"
#include <sjq_intersim.h>
#include <new_intersim_wrapper.h>
class acc : public componet
{
    using req_ptr = std::unique_ptr<cache_interface_req>;
    using req_ptr_q = std::deque<req_ptr>;
    using req_ptr_q_vec = std::vector<req_ptr_q>;

private:
    unsigned private_cache_size;
    /* data */
    unsigned num_watchers;
    unsigned num_clauses;
    std::vector<std::function<bool()>> clock_passes;
    std::vector<componet *> m_componets;
    std::vector<watcher *> watchers;
    std::vector<clause *> clauses;
    std::vector<private_cache *> m_private_caches;

    cache_interface *m_cache_interface;
    watcher_list_write_unit *m_watcher_write_unit;
    clause_writer *m_clause_write_unit;
    //icnt *m_icnt;
    icnt_mesh *m_icnt;

    //important
    //all the componets are owned by m_componnets
    //

    void init_watcher_and_clause();
    void add_hook_from_watcher_out_actions();
    void add_hook_from_clause_to_mem();
    void add_hook_from_cache_to_clause_and_watchers();
    void add_hook_from_private_cache();
    void add_hook_from_trail_to_watcher();
    void add_hook_from_clause_to_trail();
    void add_hook_from_watcher_to_writeuite();
    void add_hook_from_clause_to_writeuint();
    void add_hook_from_clause_write_unit_to_cache();
    void add_hook_from_watcher_write_unit_to_cache();
    void add_hook_from_icnt_to_other();

public:
    std::string get_internal_size() const override
    {
        std::string r;
        for (auto i : m_componets)
        {
            r.append(i->get_internal_size());
            r.append("\n");
        }
        return r;
    }

    std::string get_line_trace() const override
    {
        std::string r("start line trace..\nacc\n");
        for (auto i : m_componets)
        {

            r.append(i->get_line_trace());
            r.append("\n");
        }
        return r;
    }
    bool empty() const override
    {

        return in_m_trail.empty() and std::all_of(m_componets.begin(), m_componets.end(), [](auto &c) { return c->empty(); });
    }
    std::deque<req_ptr> in_m_trail;

    acc(unsigned num_watchers, unsigned num_clauses, uint64_t &tcurrent_cycle);
    acc(unsigned, unsigned, unsigned, unsigned, uint64_t &);
    ~acc();

protected:
    virtual bool do_cycle() override;
};

#endif