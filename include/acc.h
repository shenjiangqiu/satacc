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
class acc : public componet
{
private:
    /* data */
    unsigned num_watchers;
    unsigned num_clauses;
    std::vector<std::function<bool()>> clock_passes;
    std::vector<componet *> m_componets;
    std::vector<watcher *> watchers;
    std::vector<clause *> clauses;
    std::vector<private_cache *> m_private_caches;
    cache_interface *m_cache_interface;

    uint64_t busy = 0;
    uint64_t idle = 0;

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

    double get_busy_percent() const override
    {
        return double(busy) / double(busy + idle);
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
        return in_m_trail.empty() and std::all_of(m_componets.begin(), m_componets.end(), [](auto c) { return c->empty(); });
    }
    std::deque<cache_interface_req> in_m_trail;

    bool cycle() override;
    acc(unsigned num_watchers, unsigned num_clauses, uint64_t &tcurrent_cycle);
    ~acc();
};

#endif