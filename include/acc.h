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

#include <new_intersim_wrapper.h>
#include <ramulator_wrapper.h>
namespace sjq
{
    extern bool inside_busy;
}
class acc : public componet
{
    using req_ptr = std::unique_ptr<cache_interface_req>;
    using req_ptr_q = std::deque<req_ptr>;
    using req_ptr_q_vec = std::vector<req_ptr_q>;

private:
    unsigned long long total_memory_icnt_traffic = 0;
    unsigned long long total_watcher_clause_icnt_traffic = 0;
    unsigned long long total_watcher_writer_icnt_traffic = 0;

    void parse_file();
    unsigned private_cache_size;
    /* data */
    unsigned num_watchers;
    unsigned num_clauses;
    unsigned num_partition;
    int num_writer_entry;
    int num_writer_merge;
    std::string dram_config_file;
    //std::string watcher_icnt;
    std::vector<std::function<bool()>> clock_passes;
    std::vector<componet *> m_componets;
    std::vector<watcher *> watchers;
    std::vector<clause *> clauses;
    std::vector<private_cache *> m_private_caches;

    cache_interface *m_cache_interface;
    std::vector<watcher_list_write_unit *> m_watcher_write_unit;
    std::vector<clause_writer *> m_clause_write_unit;
    //icnt *m_icnt;
    bool enable_sequential = false;
    bool ideal_memory = false;
    bool ideal_l3cache = false;
    unsigned multi_l3cache_port = 1;
    //important
    //all the componets are owned by m_componnets
    //
    icnt_base *memory_read_icnt;

    icnt_base *watcher_to_clause_icnt;
    icnt_base *watcher_to_writer_icnt;
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

    //icnt out
    void add_hook_from_icnt_to_other();
    void add_hook_from_watcher_icnt_out();
    void add_hook_from_watcher_icnt_to_watcher_writer();

public:
    void flush_all()
    {
        for (auto &writer : m_watcher_write_unit)
        {
            writer->flush();
        }
        while (!this->empty())
        {
            this->cycle();
            this->current_cycle++;
        }
        assert(std::all_of(m_watcher_write_unit.begin(), m_watcher_write_unit.end(), [](auto &&writer) { return writer->is_flushed(); }));
    }
    std::string get_internal_size() const override
    {
        std::string r;
        for (auto i : m_componets)
        {
            r += (i->get_internal_size());
            r += ("\n");
        }
        return r;
    }

    std::string get_line_trace() const override
    {
        std::string r("start line trace..\nacc\n");
        r += fmt::format("mem_traffic: {}\nto_clause_icnt: {}\nto_writer_icnt: {}",
                         total_memory_icnt_traffic,
                         total_watcher_clause_icnt_traffic,
                         total_watcher_writer_icnt_traffic);
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