

#ifndef SJQ_INTER_SIM_H
#define SJQ_INTER_SIM_H
#include <icnt_wrapper.h>
#include <deque>
#include <component.h>
#include <cache_interface.h>
#include <memory>
#include <vector>
#include <string>
class inct : public componet
{
public:
    virtual std::string get_internal_size() const override;
    //virtual double get_busy_percent() const = 0;
    virtual std::string get_line_trace() const override;
    std::vector<std::deque<std::unique_ptr<cache_interface_req>>> in_reqs;
    std::vector<std::deque<std::unique_ptr<cache_interface_req>>> out_req;
    std::vector<std::deque<std::unique_ptr<cache_interface_req>>> in_resp;
    std::vector<std::deque<std::unique_ptr<cache_interface_req>>> out_resp;
    //static uint64_t current_cycle;
    //bool busy;
    virtual bool empty() const;
    virtual bool cycle() override;

    inct(uint64_t &current_cycle, int num_cores, int num_mems);

private:
    int n_cores;
    int n_mems;
};

#endif
