#include <sjq_intersim.h>
#include <enumerate.h>
#include <boost/log/trivial.hpp>
#include <exception>
icnt::icnt(uint64_t &current_cycle,
           int num_cores,
           int num_mem,
           int num_clauses) : componet(current_cycle),
                              n_cores(num_cores),
                              n_mems(num_mem),
                              n_clauses(num_clauses),
                              in_reqs(num_cores),
                              out_reqs(num_mem),
                              in_resps(num_mem),
                              out_resps(num_cores)
{
    //init icnt config

    global_init init;    //copy option config into the config variable
    icnt_wrapper_init(); //create the wrapper, setup this functions
    icnt_create(num_cores, num_mem);
    icnt_init();
}
std::string icnt::get_internal_size() const
{
    return fmt::format("name in_req out_req in_resp out_resp\n{} {} {} {} {}",
                       "icnt[0] ",
                       in_reqs[0].size(),
                       out_reqs[0].size(),
                       in_resps[0].size(),
                       out_resps[0].size());
}

std::string icnt::get_line_trace() const
{
    return fmt::format("name busy_percent\n{} {}", "icnt", get_busy_percent());
}

bool icnt::do_cycle()
{
    bool busy = false;
    //from in requst to icnt
    for (auto &&[i, q] : enumerate(in_reqs))
    {
        if (!q.empty())
        {
            auto &req = q.front();
            //q.pop_front();
            auto source = req->ComponentId;
            if (source >= n_cores)
            {
                //that's from clause
                int c_to_w = n_clauses / n_cores;
                source = (source - n_cores) / c_to_w;
            }

            auto size = get_pkg_size(source, n_cores + 1, req);
            auto is_has_buffer = has_buffer(source, size);
            if (is_has_buffer)
            {
                busy = true;
                auto dest = get_mem_id(req);
                push_into_inct(source, dest, std::move(req));
                q.pop_front();
            }
        }
    }

    for (auto &&[i, q] : enumerate(in_resps))
    {
        //from mem to cores

        if (!q.empty())
        {
            auto &req = q.front();
            auto resp = get_mem_id(req);
            assert(i == resp);
            auto size = get_pkg_size(resp, 0, req);
            if (has_buffer(resp, size))
            {
                busy = true;
                auto dest = req->ComponentId;
                if (dest > n_cores)
                {
                    dest = dest - n_cores;
                    auto c_to_w = n_clauses / n_cores;
                    dest = dest / c_to_w;
                }
                push_into_inct(resp, dest, std::move(req));
                q.pop_front();
            }
        }
    }

    for (auto i = 0u; i < n_cores; i++)
    {
        if (auto req_p = (cache_interface_req *)icnt_pop(i))
        {
            busy = true;
            assert(current_inflight_pkg.count(req_p) == 1);
            current_inflight_pkg.erase(req_p);
            out_resps[i].push_back(req_ptr(req_p)); //take the ownership
        }
    }

    for (auto i = 0u; i < n_mems; i++)
    {
        if (auto req_p = (cache_interface_req *)icnt_pop(n_cores + i))
        {
            busy = true;
            assert(current_inflight_pkg.count(req_p) == 1);
            current_inflight_pkg.erase(req_p);
            out_reqs[i].push_back(req_ptr((cache_interface_req *)req_p)); //take the ownership
        }
    }

    return busy;
}

unsigned icnt::get_pkg_size(unsigned source, unsigned dest, const std::unique_ptr<cache_interface_req> &req) const
{
    //push to icnt:
    auto size = 0;
    auto type = req->type;
    //auto as = req->as;
    switch (type)
    {
    case ReadType::ReadClauseData:
    case ReadType::ReadWatcher:
        if (source < dest)
        {
            size = 12;
        }
        else
        {
            size = 64;
        }

        /* code */
        break;
    case ReadType::ReadClauseValue:
    case ReadType::WatcherReadValue:
        if (source < dest)
        {
            size = 12;
        }
        else
        {
            size = 10;
        }
        /* code */
        break;
    case ReadType::writeClause:
    case ReadType::writeWatcherList:
        size = 12;
        break;
    default:
        throw std::runtime_error("no such type");
        break;
    }

    return size;
}

bool icnt::has_buffer(unsigned source, unsigned size) const
{
    return icnt_has_buffer(source, size);
}
void icnt::push_into_inct(unsigned source, unsigned dest, std::unique_ptr<cache_interface_req> req)
{

    if (source < dest)
    {
        BOOST_LOG_TRIVIAL(debug) << fmt::format("UP_TO_DOWN {} to {}", source, dest);
    }
    else if (source > dest)
    {
        BOOST_LOG_TRIVIAL(debug) << fmt::format("DOWN_TO_UP {} to {}", source, dest);
    }
    else
    {
        BOOST_LOG_TRIVIAL(fatal) << fmt::format("can't be the same: {}", source);
        throw std::runtime_error("the source and dest can't be the same of can't be the same");
    }
    auto size = get_pkg_size(source, dest, req);

    assert(icnt_has_buffer(source, size));
    current_inflight_pkg.insert(req.get());
    icnt_push(source, dest, (void *)req.release(), size); //give the ownership
}
unsigned icnt::get_mem_id(const icnt::req_ptr &req) const
{
    auto as = req->as;
    auto addr = 0ull;
    auto watcher_id = req->watcherId;
    auto clause_id = req->clauseId;
    switch (req->type)
    {
    case ReadType::ReadWatcher:
        addr = as->get_addr();
        addr += watcher_id * 8;
        /* code */
        break;
    case ReadType::WatcherReadValue:
        addr = as->get_block_addr(watcher_id);
        break;
    case ReadType::ReadClauseData:
        addr = as->get_clause_addr(watcher_id);
        break;
    case ReadType::ReadClauseValue:
        addr = as->get_clause_detail(watcher_id)[clause_id];
        break;
    case ReadType::writeClause:
        addr = as->get_clause_addr(watcher_id);
        break;
    case ReadType::writeWatcherList:
        addr = as->get_pushed_watcher_list_tail_addr(watcher_id);
        break;
    default:
        throw std::runtime_error("no such type");
        break;
    }
    auto dest = (addr >> 5) % n_mems;
    return (unsigned)dest;
}