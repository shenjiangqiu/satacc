#ifndef PREIVATE_CACHE_H
#define PREIVATE_CACHE_H
#include "component.h"
#include <sjqcache.h>
#include <deque>
#include "cache_interface.h"

//this file used for watcher's value private cache.

class private_cache : public componet
{
    using req_ptr = std::unique_ptr<cache_interface_req>;
    using req_ptr_q = std::deque<req_ptr>;
    using req_ptr_q_vec = std::vector<req_ptr_q>;
private:
    unsigned in_size = 64;
    unsigned out_size = 64;
    sjq::cache m_cache;
    std::map<uint64_t, std::vector<req_ptr>> addr_to_req;
    bool from_in_to_out();
    bool from_resp_to_send();

public:
    std::string get_line_trace() const override
    {
        auto c = *m_cache.get_stats();
        return fmt::format("{}:{} \n", "private_cache", get_busy_percent()) +
               fmt::format("c.num_hit {} ,c.num_hit_reserved {}  ,c.num_miss {} ,c.num_res_fail {} \n",
                           c.num_hit, c.num_hit_reserved, c.num_miss, c.num_res_fail);
    }

    std::string get_internal_size() const override
    {
        return fmt::format("name in out out_miss in_resp addr_to_req \n{} {} {} {} {} ", "private_cache",
                           in_request.size(),
                           out_send_q.size(),
                           out_miss_queue.size(),
                           in_resp.size(),
                           addr_to_req.size());
    }

    bool recieve_rdy()
    {
        return in_request.size() < in_size;
    }
    bool empty() const override
    {
        return in_request.empty() and
               out_send_q.empty() and
               out_miss_queue.empty() and
               in_resp.empty() and
               addr_to_req.empty();
    }
    std::deque<req_ptr> in_request;
    std::deque<req_ptr> out_send_q;//to watcher
    std::deque<req_ptr> out_miss_queue;
    std::deque<req_ptr> in_resp;

    private_cache(int asso, int total_size, uint64_t &current_cycle);

protected:
    virtual bool do_cycle() override;
};

#endif