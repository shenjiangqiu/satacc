#ifndef NEW_INTERSIM_WRAPPER_H
#define NEW_INTERSIM_WRAPPER_H

#include <deque>
#include <component.h>
#include <cache_interface.h>
#include <memory>
#include <vector>
#include <string>
#include <unordered_map>
#include <ring.h>
#include <unordered_set>
#include <mesh.h>
class icnt_ring : public componet
{
    using req_ptr = std::unique_ptr<cache_interface_req>;
    using req_ptr_q = std::deque<req_ptr>;
    using req_ptr_q_vec = std::vector<req_ptr_q>;

private:
    network_ring_c m_ring_network;
    unsigned n_cores;
    unsigned n_mems;
    unsigned n_clauses;

    void push_into_inct(bool from_cpu_to_mem, unsigned source, unsigned dest, req_ptr req); //tack the ownership
    //unsigned get_pkg_size(bool cpu_to_mem, unsigned source, unsigned dest, const req_ptr &req) const;

    std::unordered_set<cache_interface_req *> current_inflight_pkg; //do not own the ownership

public:
    bool has_pkg_in_icnt() const;
    bool has_buffer(int level, unsigned source) const;
    virtual std::string get_internal_size() const override;
    //virtual double get_busy_percent() const = 0;
    virtual std::string get_line_trace() const override;

    //interfaces
    req_ptr_q_vec in_reqs;
    req_ptr_q_vec out_reqs;
    req_ptr_q_vec in_resps;
    req_ptr_q_vec out_resps;
    //static uint64_t current_cycle;
    //bool busy;

    virtual bool empty() const
    {
        return std::all_of(in_reqs.begin(), in_reqs.end(), [](auto &q) { return q.empty(); }) and
               std::all_of(out_reqs.begin(), out_reqs.end(), [](auto &q) { return q.empty(); }) and
               std::all_of(in_resps.begin(), in_resps.end(), [](auto &q) { return q.empty(); }) and
               std::all_of(out_resps.begin(), out_resps.end(), [](auto &q) { return q.empty(); }) and
               current_inflight_pkg.empty() and !has_pkg_in_icnt();
    }

    icnt_ring(uint64_t &current_cycle,
              int num_cores,
              int num_mems,
              int n_clauses,
              int num_vc,
              int link_latency,
              int arbitration_policy,
              int link_width,
              int num_vc_cpu);
    //we don't want some global variable to init twice
    icnt_ring() = delete;
    icnt_ring(const icnt_ring &) = delete;
    icnt_ring(icnt_ring &&) = delete;

protected:
    virtual bool do_cycle() override;
};

class icnt_ideal : public componet
{
    using req_ptr = std::unique_ptr<cache_interface_req>;
    using req_ptr_q = std::deque<req_ptr>;
    using req_ptr_q_vec = std::vector<req_ptr_q>;
    unsigned n_cores;
    unsigned n_mems;
    unsigned n_clauses;

public:
    bool has_pkg_in_icnt() const;
    bool has_buffer(int level, unsigned source) const;
    virtual std::string get_internal_size() const override;
    //virtual double get_busy_percent() const = 0;
    virtual std::string get_line_trace() const override;

    //interfaces
    req_ptr_q_vec in_reqs;
    req_ptr_q_vec out_reqs;
    req_ptr_q_vec in_resps;
    req_ptr_q_vec out_resps;
    //static uint64_t current_cycle;
    //bool busy;

    virtual bool empty() const;

    icnt_ideal(uint64_t &current_cycle,
               unsigned num_cores,
               unsigned num_mems,
               unsigned num_clauses);
    //we don't want some global variable to init twice
    icnt_ideal() = delete;
    icnt_ideal(const icnt_ideal &) = delete;
    icnt_ideal(icnt_ideal &&) = delete;

protected:
    virtual bool do_cycle() override;
};

class icnt_mesh : public componet
{
    using req_ptr = std::unique_ptr<cache_interface_req>;
    using req_ptr_q = std::deque<req_ptr>;
    using req_ptr_q_vec = std::vector<req_ptr_q>;

private:
    network_mesh_c m_mesh_network;
    unsigned n_cores;
    unsigned n_mems;
    unsigned n_clauses;

    void push_into_inct(bool from_cpu_to_mem, unsigned source, unsigned dest, req_ptr req); //tack the ownership
    //unsigned get_pkg_size(bool cpu_to_mem, unsigned source, unsigned dest, const req_ptr &req) const;

    std::unordered_set<cache_interface_req *> current_inflight_pkg; //do not own the ownership

public:
    bool has_pkg_in_icnt() const;
    bool has_buffer(int level, unsigned source) const;
    virtual std::string get_internal_size() const override;
    //virtual double get_busy_percent() const = 0;
    virtual std::string get_line_trace() const override;

    //interfaces
    req_ptr_q_vec in_reqs;
    req_ptr_q_vec out_reqs;
    req_ptr_q_vec in_resps;
    req_ptr_q_vec out_resps;
    //static uint64_t current_cycle;
    //bool busy;

    virtual bool empty() const
    {
        return std::all_of(in_reqs.begin(), in_reqs.end(), [](auto &q) { return q.empty(); }) and
               std::all_of(out_reqs.begin(), out_reqs.end(), [](auto &q) { return q.empty(); }) and
               std::all_of(in_resps.begin(), in_resps.end(), [](auto &q) { return q.empty(); }) and
               std::all_of(out_resps.begin(), out_resps.end(), [](auto &q) { return q.empty(); }) and
               current_inflight_pkg.empty() and !has_pkg_in_icnt();
    }

    icnt_mesh(uint64_t &current_cycle,
              int num_cores,
              int num_mems,
              int n_clauses,
              int num_vc,
              int link_latency,
              int arbitration_policy,
              int link_width,
              int num_vc_cpu);
    //we don't want some global variable to init twice
    icnt_mesh() = delete;
    icnt_mesh(const icnt_mesh &) = delete;
    icnt_mesh(icnt_mesh &&) = delete;

protected:
    virtual bool do_cycle() override;
};

#endif