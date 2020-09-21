#include <sjq_intersim.h>
#include <enumerate.h>
#include <boost/log/trivial.hpp>
#include <exception>
#include <req_addr.h>
#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>

namespace logging = boost::log;
void init_log()
{
    logging::core::get()->set_filter(
        logging::trivial::severity >= logging::trivial::error);
}
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
    init_log();
}
std::string icnt::get_internal_size() const
{

    std::string ret;
    for (auto i = 0u; i < n_cores; i++)
    {
        ret += fmt::format("name id in_req out_req \n{}-{} {} {}\n",
                           "icnt", i,
                           in_reqs[i].size(),
                           out_resps[i].size());
    }
    for (auto i = 0u; i < n_mems; i++)
    {
        ret += fmt::format("name id in_resp out_resp \n{}-{} {} {}\n",
                           "icnt", i,
                           in_resps[i].size(),
                           out_reqs[i].size());
    }
    ret += fmt::format("\ncurrent_inflight: {}\n", current_inflight_pkg.size());
    if (icnt_busy())
    {
        ret += "\n ICNT BUSY!!\n";
    }
    return ret;
}

std::string icnt::get_line_trace() const
{
    return fmt::format("name busy_percent\n{} {}", "icnt", get_busy_percent());
}

bool icnt::do_cycle()
{
    icnt_transfer();

    bool busy = false;
    //from in requst to icnt
    for (auto &&[i, q] : enumerate(in_reqs))
    {
        if (!q.empty())
        {
            auto &req = q.front();
            assert(req);
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
                auto mem_partition_id = get_partition_id_by_addr(get_addr_by_req(req), n_mems);
                auto dest = mem_partition_id + n_cores;
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
            auto resp = get_partition_id_by_addr(get_addr_by_req(req), n_mems);
            assert(i == resp);
            auto size = get_pkg_size(resp, 0, req);
            auto source = resp + n_cores;
            //fix bug here, should test source not the dst
            if (has_buffer(source, size))
            {
                busy = true;
                auto dest = req->ComponentId;
                //fix bug here, this is >= not >!!!
                if (dest >= n_cores)
                {
                    dest = dest - n_cores;
                    auto c_to_w = n_clauses / n_cores;
                    dest = dest / c_to_w;
                }
                push_into_inct(source, dest, std::move(req));
                q.pop_front();
            }
        }
    }

    for (auto i = 0u; i < n_cores; i++)
    {
        if (auto req_p = (cache_interface_req *)icnt_pop(i))
        {
            BOOST_LOG_TRIVIAL(debug) << fmt::format("ICNT OUT: FROM PORT {}", i);
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
            BOOST_LOG_TRIVIAL(debug) << fmt::format("ICNT OUT: FROM PORT {}", n_cores + i);
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
    case AccessType::ReadClauseData:
    case AccessType::ReadWatcherData:
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
    case AccessType::ReadClauseValue:
    case AccessType::ReadWatcherValue:
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
    case AccessType::writeClause:
    case AccessType::writeWatcherList:
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
