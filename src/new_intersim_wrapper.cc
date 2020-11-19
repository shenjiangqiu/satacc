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
unsigned int get_pkg_size(const std::unique_ptr<cache_interface_req> &req)
{
    return req->m_size;
}
namespace logging = boost::log;
icnt_ring::icnt_ring(uint64_t &current_cycle,
                     unsigned n_nodes,
                     int num_vc,
                     int link_latency,
                     int arbitration_policy,
                     int link_width,
                     int num_vc_cpu) : icnt_base(current_cycle, n_nodes),
                                       m_ring_network(current_cycle,
                                                      num_vc, link_latency,
                                                      arbitration_policy,
                                                      link_width,
                                                      num_vc_cpu)
{
    //init icnt config
    throw;
    //not finished!!!
    m_ring_network.init(n_nodes);
    logging::core::get()->set_filter(
        logging::trivial::severity >= logging::trivial::error);
}
bool icnt_ring::has_pkg_in_icnt() const
{
    return !current_inflight_pkg.empty();
}
std::string icnt_ring::get_internal_size() const
{
    throw;
    //not finished
    std::string ret;
    for (auto i = 0u; i < n_nodes; i++)
    {
        ret += fmt::format("name id in_req out_req \n{}-{} {} {}\n",
                           "icnt", i,
                           in_reqs[i].size(),
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
    //TODO not finished
    throw;
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
            if (source >= n_nodes)
            {
                //that's from clause
                int c_to_w = n_nodes / n_nodes;
                source = (source - n_nodes) / c_to_w;
            }
            assert(source == i);

            auto size = get_pkg_size(req);
            req->m_size = size;
            auto is_has_buffer = has_buffer(n_nodes);

            if (is_has_buffer)
            {
                busy = true;
                auto mem_partition_id = get_partition_id_by_addr(get_addr_by_req(req), n_nodes);
                //auto dest = mem_partition_id;
                push_into_inct(source, n_nodes, std::move(req));
                q.pop_front();
            }
        }
    }

    for (auto i = 0u; i < n_nodes; i++)
    {
        if (auto req_p = m_ring_network.receive(i))
        {
            m_ring_network.receive_pop(i);
            BOOST_LOG_TRIVIAL(debug) << fmt::format("ICNT OUT: FROM PORT {}", i);
            busy = true;
            assert(current_inflight_pkg.count(req_p) == 1);
            current_inflight_pkg.erase(req_p);
            out_reqs[i].push_back(req_ptr(req_p)); //take the ownership
        }
    }

    for (auto i = 0u; i < n_nodes; i++)
    {
        if (auto req_p = m_ring_network.receive(i))
        {
            m_ring_network.receive_pop(i);
            BOOST_LOG_TRIVIAL(debug) << fmt::format("ICNT OUT: FROM PORT {}", n_nodes + i);
            busy = true;
            assert(current_inflight_pkg.count(req_p) == 1);
            current_inflight_pkg.erase(req_p);
            out_reqs[i].push_back(req_ptr((cache_interface_req *)req_p)); //take the ownership
        }
    }

    return busy;
}

bool icnt_ring::has_buffer(unsigned source) const
{
    return m_ring_network.has_buffer(source);
}

void icnt_ring::push_into_inct(unsigned source, unsigned dest, std::unique_ptr<cache_interface_req> req)
{

    current_inflight_pkg.insert(req.get());
    //icnt_push(source, dest, (void *)req.release(), size); //give the ownership
    m_ring_network.send(req.release(), source, dest);
}

// mext it's mesh

//TODO make a base class for some function

icnt_mesh::icnt_mesh(uint64_t &current_cycle,
                     unsigned n_nodes,
                     int num_vc,
                     int link_latency,
                     int arbitration_policy,
                     int link_width,
                     int num_vc_cpu) : icnt_base(current_cycle, n_nodes),
                                       m_mesh_network(current_cycle,
                                                      num_vc, link_latency,
                                                      arbitration_policy,
                                                      link_width,
                                                      num_vc_cpu)
{
    //init icnt config
    m_mesh_network.init(n_nodes);
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
    for (auto i = 0u; i < n_nodes; i++)
    {
        ret += fmt::format("name id in_req out_req \n{}-{} {} {}\n",
                           "icnt", i,
                           in_reqs[i].size(),
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
            auto source = req->icnt_from;
            assert(source != req->icnt_to);
            assert((size_t)source == i);
            auto is_has_buffer = has_buffer(source);

            if (is_has_buffer)
            {
                busy = true;
                int dest = 0;
                dest = req->icnt_to;

                //auto dest = mem_partition_id;
                push_into_inct(source, dest, std::move(req));
                q.pop_front();
            }
        }
    }
    for (auto i = 0u; i < n_nodes; i++)
    {
        if (auto req_p = m_mesh_network.receive(i))
        {
            m_mesh_network.receive_pop(i);
            BOOST_LOG_TRIVIAL(debug) << fmt::format("ICNT OUT: FROM PORT {}", i);
            busy = true;
            assert(current_inflight_pkg.count(req_p) == 1);
            current_inflight_pkg.erase(req_p);
            out_reqs[i].push_back(req_ptr(req_p)); //take the ownership
        }
    }

    return busy;
}

bool icnt_mesh::has_buffer(unsigned source) const
{
    return m_mesh_network.has_buffer(source);
}

void icnt_mesh::push_into_inct(unsigned source, unsigned dest, std::unique_ptr<cache_interface_req> req)
{

    current_inflight_pkg.insert(req.get());
    //icnt_push(source, dest, (void *)req.release(), size); //give the ownership
    auto result = m_mesh_network.send(req.release(), source, dest);
    assert(result);
}

bool icnt_ideal::has_pkg_in_icnt() const
{
    return empty();
}
bool icnt_ideal::has_buffer(unsigned int source) const
{
    bool result;
    result = in_reqs[source].size() < 16;
    return result;
}
std::string icnt_ideal::get_internal_size() const
{

    std::string ret;
    for (auto i = 0u; i < n_nodes; i++)
    {
        ret += fmt::format("name id in_req out_req \n{}-{} {} {}\n",
                           "icnt", i,
                           in_reqs[i].size(),
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
    auto result = std::all_of(in_reqs.begin(), in_reqs.end(), [](auto &q) { return q.empty(); }) and
                  std::all_of(out_reqs.begin(), out_reqs.end(), [](auto &q) { return q.empty(); });
    //std::cout<<result<<std::endl;
    return result;
}
icnt_ideal::icnt_ideal(uint64_t &current_cycle, unsigned num_nodes)
    : icnt_base(current_cycle, num_nodes) {}

bool icnt_ideal::do_cycle()
{
    for (auto &&[i, q] : enumerate(in_reqs))
    {
        while (!q.empty())
        {
            busy = true;
            auto &req = q.front();
            assert(req);
            //q.pop_front();

            out_reqs[req->icnt_to].push_back(std::move(req));
            q.pop_front();
        }
    }

    return busy;
}
icnt_base::icnt_base(uint64_t &current_cycle,
                     unsigned n_nodes) : componet(current_cycle),
                                         n_nodes(n_nodes),
                                         in_reqs(n_nodes),
                                         out_reqs(n_nodes) {}
