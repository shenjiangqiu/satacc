#include <new_intersim_wrapper.h>
#include <enumerate.h>
#include <boost/log/trivial.hpp>
#include <exception>
#include <req_addr.h>
#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <network.h>
#include <ring.h>
#include <memreq_info.h>
unsigned int get_pkg_size(bool cpu_to_mem, unsigned int, unsigned int, const std::unique_ptr<cache_interface_req> &req)
{
    auto size = 0;
    auto type = req->type;
    //auto as = req->as;
    switch (type)
    {
    case AccessType::ReadClauseData:
    case AccessType::ReadWatcherData:
        if (cpu_to_mem)
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
        if (cpu_to_mem)
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
namespace logging = boost::log;
icnt_ring::icnt_ring(uint64_t &current_cycle,
                     int num_cores,
                     int num_mem,
                     int num_clauses,
                     int num_vc,
                     int link_latency,
                     int arbitration_policy,
                     int link_width,
                     int num_vc_cpu) : icnt_base(current_cycle, num_cores, num_mem),
                                       m_ring_network(current_cycle,
                                                      num_vc, link_latency,
                                                      arbitration_policy,
                                                      link_width,
                                                      num_vc_cpu),
                                       n_cores(num_cores),
                                       n_mems(num_mem),
                                       n_clauses(num_clauses)
{
    //init icnt config
    m_ring_network.init(num_cores, 0, num_mem, 0, 0);
    logging::core::get()->set_filter(
        logging::trivial::severity >= logging::trivial::error);
}
bool icnt_ring::has_pkg_in_icnt() const
{
    return !current_inflight_pkg.empty();
}
std::string icnt_ring::get_internal_size() const
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
    if (!current_inflight_pkg.empty())
    {
        ret += "\n ICNT BUSY!!\n";
    }
    return ret;
}

std::string icnt_ring::get_line_trace() const
{
    return fmt::format("name busy_percent\n{} {}", "icnt", get_busy_percent());
}

bool icnt_ring::do_cycle()
{
    m_ring_network.run_a_cycle(false);

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
            assert(source == i);

            auto size = get_pkg_size(true, source, n_cores + 1, req);
            req->m_size = size;
            auto is_has_buffer = has_buffer(MEM_L2, source);

            if (is_has_buffer)
            {
                busy = true;
                auto mem_partition_id = get_partition_id_by_addr(get_addr_by_req(req), n_mems);
                //auto dest = mem_partition_id;
                push_into_inct(true, source, mem_partition_id, std::move(req));
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
            assert(i == get_partition_id_by_addr(get_addr_by_req(req), n_mems));
            auto size = get_pkg_size(false, i, 0, req);
            req->m_size = size;
            if (has_buffer(MEM_L3, i))
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
                //auto source = resp + n_cores;
                push_into_inct(false, i, dest, std::move(req));
                q.pop_front();
            }
        }
    }

    for (auto i = 0u; i < n_cores; i++)
    {
        if (auto req_p = m_ring_network.receive(MEM_L2, i))
        {
            m_ring_network.receive_pop(MEM_L2, i);
            BOOST_LOG_TRIVIAL(debug) << fmt::format("ICNT OUT: FROM PORT {}", i);
            busy = true;
            assert(current_inflight_pkg.count(req_p) == 1);
            current_inflight_pkg.erase(req_p);
            out_resps[i].push_back(req_ptr(req_p)); //take the ownership
        }
    }

    for (auto i = 0u; i < n_mems; i++)
    {
        if (auto req_p = m_ring_network.receive(MEM_L3, i))
        {
            m_ring_network.receive_pop(MEM_L3, i);
            BOOST_LOG_TRIVIAL(debug) << fmt::format("ICNT OUT: FROM PORT {}", n_cores + i);
            busy = true;
            assert(current_inflight_pkg.count(req_p) == 1);
            current_inflight_pkg.erase(req_p);
            out_reqs[i].push_back(req_ptr((cache_interface_req *)req_p)); //take the ownership
        }
    }

    return busy;
}

bool icnt_ring::has_buffer(int level, unsigned source) const
{
    return m_ring_network.has_buffer(level, source);
}

void icnt_ring::push_into_inct(bool cpu_to_mem, unsigned source, unsigned dest, std::unique_ptr<cache_interface_req> req)
{

    auto size = get_pkg_size(cpu_to_mem, source, dest, req);
    auto srclevel = cpu_to_mem ? MEM_L2 : MEM_L3;
    auto dstlevel = cpu_to_mem ? MEM_L3 : MEM_L2;
    req->m_size = size;

    current_inflight_pkg.insert(req.get());
    //icnt_push(source, dest, (void *)req.release(), size); //give the ownership
    m_ring_network.send(req.release(), srclevel, source, dstlevel, dest);
}

// mext it's mesh

//TODO make a base class for some function

icnt_mesh::icnt_mesh(uint64_t &current_cycle,
                     int num_cores,
                     int num_mem,
                     int num_clauses,
                     int num_vc,
                     int link_latency,
                     int arbitration_policy,
                     int link_width,
                     int num_vc_cpu) : icnt_base(current_cycle, num_cores, num_mem),
                                       m_mesh_network(current_cycle,
                                                      num_vc, link_latency,
                                                      arbitration_policy,
                                                      link_width,
                                                      num_vc_cpu),
                                       n_cores(num_cores),
                                       n_mems(num_mem),
                                       n_clauses(num_clauses)
{
    //init icnt config
    m_mesh_network.init(num_cores, 0, num_mem, 0, 0);
    logging::core::get()->set_filter(
        logging::trivial::severity >= logging::trivial::error);
}
bool icnt_mesh::has_pkg_in_icnt() const
{
    return !current_inflight_pkg.empty();
}
std::string icnt_mesh::get_internal_size() const
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
    if (!current_inflight_pkg.empty())
    {
        ret += "\n ICNT BUSY!!\n";
    }
    return ret;
}

std::string icnt_mesh::get_line_trace() const
{
    return fmt::format("name busy_percent\n{} {}", "icnt", get_busy_percent());
}

bool icnt_mesh::do_cycle()
{
    m_mesh_network.run_a_cycle(false);

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
            assert(source == i);

            auto size = get_pkg_size(true, source, n_cores + 1, req);
            req->m_size = size;
            auto is_has_buffer = has_buffer(MEM_L2, source);

            if (is_has_buffer)
            {
                busy = true;
                auto mem_partition_id = get_partition_id_by_addr(get_addr_by_req(req), n_mems);
                //auto dest = mem_partition_id;
                push_into_inct(true, source, mem_partition_id, std::move(req));
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
            assert(i == get_partition_id_by_addr(get_addr_by_req(req), n_mems));
            auto size = get_pkg_size(false, i, 0, req);
            req->m_size = size;
            if (has_buffer(MEM_L3, i))
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
                //auto source = resp + n_cores;
                push_into_inct(false, i, dest, std::move(req));
                q.pop_front();
            }
        }
    }

    for (auto i = 0u; i < n_cores; i++)
    {
        if (auto req_p = m_mesh_network.receive(MEM_L2, i))
        {
            m_mesh_network.receive_pop(MEM_L2, i);
            BOOST_LOG_TRIVIAL(debug) << fmt::format("ICNT OUT: FROM PORT {}", i);
            busy = true;
            assert(current_inflight_pkg.count(req_p) == 1);
            current_inflight_pkg.erase(req_p);
            out_resps[i].push_back(req_ptr(req_p)); //take the ownership
        }
    }

    for (auto i = 0u; i < n_mems; i++)
    {
        if (auto req_p = m_mesh_network.receive(MEM_L3, i))
        {
            m_mesh_network.receive_pop(MEM_L3, i);
            BOOST_LOG_TRIVIAL(debug) << fmt::format("ICNT OUT: FROM PORT {}", n_cores + i);
            busy = true;
            assert(current_inflight_pkg.count(req_p) == 1);
            current_inflight_pkg.erase(req_p);
            out_reqs[i].push_back(req_ptr((cache_interface_req *)req_p)); //take the ownership
        }
    }

    return busy;
}

bool icnt_mesh::has_buffer(int level, unsigned source) const
{
    return m_mesh_network.has_buffer(level, source);
}

void icnt_mesh::push_into_inct(bool cpu_to_mem, unsigned source, unsigned dest, std::unique_ptr<cache_interface_req> req)
{

    auto size = get_pkg_size(cpu_to_mem, source, dest, req);
    auto srclevel = cpu_to_mem ? MEM_L2 : MEM_L3;
    auto dstlevel = cpu_to_mem ? MEM_L3 : MEM_L2;
    req->m_size = size;

    current_inflight_pkg.insert(req.get());
    //icnt_push(source, dest, (void *)req.release(), size); //give the ownership
    m_mesh_network.send(req.release(), srclevel, source, dstlevel, dest);
}

bool icnt_ideal::has_pkg_in_icnt() const
{
    return empty();
}
bool icnt_ideal::has_buffer(int level, unsigned int source) const
{
    return level == 0 ? in_reqs[source].size() <= 16 : in_resps[source].size() <= 16;
}
std::string icnt_ideal::get_internal_size() const
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
    return ret;
}
std::string icnt_ideal::get_line_trace() const
{

    return fmt::format("name busy_percent\n{} {}", "icnt", get_busy_percent());
}
bool icnt_ideal::empty() const
{
    return std::all_of(in_reqs.begin(), in_reqs.end(), [](auto &q) { return q.empty(); }) and
           std::all_of(out_reqs.begin(), out_reqs.end(), [](auto &q) { return q.empty(); }) and
           std::all_of(in_resps.begin(), in_resps.end(), [](auto &q) { return q.empty(); }) and
           std::all_of(out_resps.begin(), out_resps.end(), [](auto &q) { return q.empty(); });
}
icnt_ideal::icnt_ideal(uint64_t &current_cycle, unsigned num_cores, unsigned num_mems, unsigned num_clauses)
    : icnt_base(current_cycle, num_cores, num_mems), n_cores(num_cores), n_mems(num_mems), n_clauses(num_clauses) {}

bool icnt_ideal::do_cycle()
{
    for (auto &&[i, q] : enumerate(in_reqs))
    {
        if (!q.empty())
        {
            busy = true;
            auto &req = q.front();
            assert(req);
            //q.pop_front();

            auto size = get_pkg_size(true, i, n_cores + 1, req);
            req->m_size = size;
            auto mem_partition_id = get_partition_id_by_addr(get_addr_by_req(req), n_mems);
            out_reqs[mem_partition_id].push_back(std::move(req));
            q.pop_front();
        }
    }

    for (auto &&[i, q] : enumerate(in_resps))
    {
        //from mem to cores

        if (!q.empty())
        {
            auto &req = q.front();
            assert(i == get_partition_id_by_addr(get_addr_by_req(req), n_mems));
            auto size = get_pkg_size(false, i, 0, req);
            req->m_size = size;
            busy = true;
            auto dest = req->ComponentId;
            //fix bug here, this is >= not >!!!
            if (dest >= n_cores)
            {
                dest = dest - n_cores;
                auto c_to_w = n_clauses / n_cores;
                dest = dest / c_to_w;
            }
            //auto source = resp + n_cores;
            out_resps[dest].push_back(std::move(req));

            q.pop_front();
        }
    }
    return busy;
}
icnt_base::icnt_base(uint64_t &current_cycle,
                     unsigned int num_core,
                     unsigned int num_mem) : componet(current_cycle),
                                             in_reqs(num_core),
                                             out_reqs(num_mem),
                                             in_resps(num_mem),
                                             out_resps(num_core) {}
