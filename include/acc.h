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

public:
    bool empty()
    {
        return in_m_trail.empty() and
               std::all_of(watchers.begin(), watchers.end(), [](auto w) { return w->empty(); }) and
               std::all_of(clauses.begin(), clauses.end(), [](auto c) { return c->empty(); });
    }
    std::deque<cache_interface_req> in_m_trail;

    bool cycle() override;
    acc(unsigned num_watchers, unsigned num_clauses);
    ~acc();
};

#endif