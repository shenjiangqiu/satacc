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
class icnt_base : public componet
{
    using req_ptr = std::unique_ptr<cache_interface_req>;
    using req_ptr_q = std::deque<req_ptr>;
    using req_ptr_q_vec = std::vector<req_ptr_q>;


protected:
    unsigned n_nodes;

public:
    //interfaces
    req_ptr_q_vec in_reqs;
    req_ptr_q_vec out_reqs;
    icnt_base(uint64_t &current_cycle,
              unsigned num_nodes);
    virtual bool has_pkg_in_icnt() const = 0;
    virtual bool has_buffer(unsigned source) const = 0;
    virtual ~icnt_base() {}
};

class icnt_ring : public icnt_base
{
    using req_ptr = std::unique_ptr<cache_interface_req>;
    using req_ptr_q = std::deque<req_ptr>;
    using req_ptr_q_vec = std::vector<req_ptr_q>;

private:
    network_ring_c m_ring_network;
    unsigned n_nodes;

    void push_into_inct(unsigned source, unsigned dest, req_ptr req); //tack the ownership
    //unsigned get_pkg_size(bool cpu_to_mem, unsigned source, unsigned dest, const req_ptr &req) const;

    std::unordered_set<cache_interface_req *> current_inflight_pkg; //do not own the ownership

public:
    bool has_pkg_in_icnt() const override;
    bool has_buffer(unsigned source) const override;
    virtual std::string get_internal_size() const override;
    //virtual double get_busy_percent() const = 0;
    virtual std::string get_line_trace() const override;

    //static uint64_t current_cycle;
    //bool busy;

    virtual bool empty() const override
    {
        return std::all_of(in_reqs.begin(), in_reqs.end(), [](auto &q) { return q.empty(); }) and
               std::all_of(out_reqs.begin(), out_reqs.end(), [](auto &q) { return q.empty(); }) and
               current_inflight_pkg.empty() and !has_pkg_in_icnt();
    }

    icnt_ring(uint64_t &current_cycle,
              unsigned n_nodes,
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

class icnt_ideal : public icnt_base
{
    using req_ptr = std::unique_ptr<cache_interface_req>;
    using req_ptr_q = std::deque<req_ptr>;
    using req_ptr_q_vec = std::vector<req_ptr_q>;

public:
    bool has_pkg_in_icnt() const override;
    bool has_buffer(unsigned source) const override;
    virtual std::string get_internal_size() const override;
    //virtual double get_busy_percent() const = 0;
    virtual std::string get_line_trace() const override;

    //static uint64_t current_cycle;
    //bool busy;

    virtual bool empty() const override;

    icnt_ideal(uint64_t &current_cycle, unsigned n_nodes);
    //we don't want some global variable to init twice
    icnt_ideal() = delete;
    icnt_ideal(const icnt_ideal &) = delete;
    icnt_ideal(icnt_ideal &&) = delete;

protected:
    virtual bool do_cycle() override;
};

class icnt_mesh : public icnt_base
{
    using req_ptr = std::unique_ptr<cache_interface_req>;
    using req_ptr_q = std::deque<req_ptr>;
    using req_ptr_q_vec = std::vector<req_ptr_q>;

private:
    network_mesh_c m_mesh_network;
    bool push_into_inct(unsigned source, unsigned dest, req_ptr req); //tack the ownership
    //unsigned get_pkg_size(bool cpu_to_mem, unsigned source, unsigned dest, const req_ptr &req) const;

    std::unordered_set<cache_interface_req *> current_inflight_pkg; //do not own the ownership

public:
    bool has_pkg_in_icnt() const override;
    bool has_buffer(unsigned source) const override;
    virtual std::string get_internal_size() const override;
    //virtual double get_busy_percent() const = 0;
    virtual std::string get_line_trace() const override;

    //static uint64_t current_cycle;
    //bool busy;

    virtual bool empty() const override
    {
        return std::all_of(in_reqs.begin(), in_reqs.end(), [](auto &q) { return q.empty(); }) and
               std::all_of(out_reqs.begin(), out_reqs.end(), [](auto &q) { return q.empty(); }) and
               current_inflight_pkg.empty() and !has_pkg_in_icnt();
    }

    icnt_mesh(uint64_t &current_cycle,
              unsigned n_nodes,
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