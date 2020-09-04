#include <sjq_intersim.h>
#include <enumerate.h>
inct::inct(uint64_t &current_cycle,
           int num_cores,
           int num_mem) : componet(current_cycle),
                          n_cores(num_cores),
                          n_mems(num_mem)
{
    //init icnt config

    global_init init;    //copy option config into the config variable
    icnt_wrapper_init(); //create the wrapper
    icnt_create(num_cores, num_mem);
    icnt_init();
}
std::string inct::get_internal_size() const
{
    return fmt::format("name in_req out_req in_resp out_resp\n{} {} {} {} {}",
                       "icnt[0] ", in_reqs[0].size(),
                       out_req.size(),
                       in_resp.size(),
                       out_resp.size());
}

std::string inct::get_line_trace() const
{
    return fmt::format("name busy_percent\n{} {}", "icnt", get_busy_percent());
}

bool inct::cycle()
{
    bool busy = false;

    for (auto &&[i,q] :enumerate(in_reqs))
    {
        if(!q.empty()){
            
        }

    }
    for (auto &q : in_resp)
    {
    }
    return busy;
}
