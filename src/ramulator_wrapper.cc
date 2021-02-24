#include <ramulator_wrapper.h>
#include <MemoryFactory.h>
#include <map>
#include "RamulatorConfig.h"
#include "Request.h"
#include "MemoryFactory.h"
#include "Memory.h"
#include "DDR3.h"
#include "DDR4.h"
#include "LPDDR3.h"
#include "LPDDR4.h"
#include "GDDR5.h"
#include "WideIO.h"
#include "WideIO2.h"
#include "HBM.h"
#include "SALP.h"
#include <fmt/format.h>
using namespace ramulator;

static map<string, function<MemoryBase *(const Config &, int)>> name_to_func = {
    {"DDR3", &MemoryFactory<DDR3>::create},
    {"DDR4", &MemoryFactory<DDR4>::create},
    {"LPDDR3", &MemoryFactory<LPDDR3>::create},
    {"LPDDR4", &MemoryFactory<LPDDR4>::create},
    {"GDDR5", &MemoryFactory<GDDR5>::create},
    {"WideIO", &MemoryFactory<WideIO>::create},
    {"WideIO2", &MemoryFactory<WideIO2>::create},
    {"HBM", &MemoryFactory<HBM>::create},
    {"SALP-1", &MemoryFactory<SALP>::create},
    {"SALP-2", &MemoryFactory<SALP>::create},
    {"SALP-MASA", &MemoryFactory<SALP>::create},
};

ramulator_wrapper::ramulator_wrapper(const ramulator::Config &configs, int cacheline, uint64_t &t_current_cycle) : componet(t_current_cycle)
{
    const string &std_name = configs["standard"];
    assert(name_to_func.find(std_name) != name_to_func.end() && "unrecognized standard name");
    mem = name_to_func[std_name](configs, cacheline);
    Stats::statlist.output("mem_stats.txt");
    tCK = mem->clk_ns();
}
ramulator_wrapper::~ramulator_wrapper()
{
    Stats::statlist.printall();
    mem->finish();
    delete mem;
}
void ramulator_wrapper::finish()
{
    mem->finish();
}
void ramulator_wrapper::tick()
{
    mem->tick();
}
void ramulator_wrapper::send(uint64_t addr, bool is_read)
{
    this->in_queue.push({addr, is_read});
}
void ramulator_wrapper::call_back(ramulator::Request &req)
{
    outgoing_reqs--;
    assert((long long)outgoing_reqs >= 0);
    switch (req.type)
    {
    case Request::Type::READ:
        out_queue.push(req.addr);
        break;

    default:
        assert(req.type == Request::Type::WRITE);
        break;
    }
}
bool ramulator_wrapper::do_cycle()
{
    bool busy = false;
    if (!in_queue.empty())
    {
        busy = true;
        auto &req = in_queue.front();
        auto r_req = Request(req.first, req.second ? Request::Type::READ : Request::Type::WRITE, [this](Request &req) { this->call_back(req); });
        if (mem->send(r_req))
        {
            outgoing_reqs++;
            in_queue.pop();
        }
    }
    this->tick();
    return busy;
}
bool ramulator_wrapper::empty() const
{
    return in_queue.empty() and out_queue.empty() and outgoing_reqs == 0;
}
std::string ramulator_wrapper::get_internal_size() const
{
    return fmt::format("name in out outgoing\n mem {} {} {}", in_queue.size(), out_queue.size(), outgoing_reqs);
}

std::string ramulator_wrapper::get_line_trace() const
{
    return fmt::format("{}", get_busy_percent());
}
