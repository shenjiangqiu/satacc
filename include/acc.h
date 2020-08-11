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
#include <fmt/core.h>

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
    cache_interface *m_cache_interface;

    uint64_t busy = 0;
    uint64_t idle = 0;

public:
    std::string get_internal_size()
    {
        std::string r;
        r.append("acc\n");
        r.append("\ncache\n");
        r.append("\n");
        r.append("\t");
        r.append(m_cache_interface->get_internal_size());
        r.append("\nwatchers:\n");
        for (auto watcher : watchers)
        {
            r.append("\t");
            r.append(watcher->get_internal_size());
            r.append("\n");
        }
        r.append("\n");
        r.append("\nclauses:\n");

        for (auto clause : clauses)
        {
            r.append("\t");
            r.append(clause->get_internal_size());
            r.append("\n");
        }
        return r;
    }

    double get_busy_percent()
    {
        return double(busy) / double(busy + idle);
    }
    std::string get_line_trace()
    {
        std::string linetrace = std::to_string(get_busy_percent());
        linetrace.append("\n");
        linetrace.append("watchers\n");
        for (unsigned i = 0; i < num_watchers; i++)
        {
            linetrace.append("\t");
            linetrace.append(watchers[i]->get_line_trace());
            linetrace.append("\n");
        }
        linetrace.append("clauses\n");

        for (unsigned i = 0; i < num_clauses; i++)
        {
            linetrace.append("\t");
            linetrace.append(clauses[i]->get_line_trace());
            linetrace.append("\n");
        }
        linetrace.append("cache\n");
        linetrace.append("\t");
        linetrace.append(m_cache_interface->get_line_trace());
        return linetrace;
    }
    bool empty()
    {
        return in_m_trail.empty() and m_cache_interface->empty() and
               std::all_of(watchers.begin(), watchers.end(), [](auto w) { return w->empty(); }) and
               std::all_of(clauses.begin(), clauses.end(), [](auto c) { return c->empty(); });
    }
    std::deque<cache_interface_req> in_m_trail;

    bool cycle() override;
    acc(unsigned num_watchers, unsigned num_clauses);
    ~acc();
};

#endif