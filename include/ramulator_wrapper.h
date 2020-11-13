#ifndef RAMULATOR_WRAPPER_H
#define RAMULATOR_WRAPPER_H
#include "Cache.h"
#include "RamulatorConfig.h"
#include "Memory.h"
#include "Request.h"
#include "Statistics.h"
#include <iostream>
#include <vector>
#include <fstream>
#include <string>
#include <ctype.h>
#include <functional>
#include <component.h>
#include <queue>
#include <tuple>
#include <map>
#include <set>
class ramulator_wrapper : public componet
{
public:
    void send(uint64_t addr, bool is_read);
    void tick();
    void finish();
    ramulator_wrapper(const ramulator::Config &configs, int cacheline, uint64_t &t_current_cycle);
    ~ramulator_wrapper();
    void call_back(ramulator::Request &req);
    bool empty() const override;
    std::string get_internal_size() const override;
    std::string get_line_trace() const override;

    uint64_t get() const
    {
        return out_queue.front();
    }
    uint64_t pop()
    {
        auto ret = out_queue.front();
        out_queue.pop();
        return ret;
    }
    bool return_avaliable() const
    {
        return !out_queue.empty();
    }

protected:
    bool do_cycle() override;

private:
    double tCK;
    unsigned long long outgoing_reqs = 0;
    std::queue<std::pair<uint64_t, bool>>
        in_queue;
    std::queue<uint64_t> out_queue;
    ramulator::MemoryBase *mem;
};

#endif