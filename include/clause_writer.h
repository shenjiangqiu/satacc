#ifndef CLAUSE_WRITER_H
#define CLAUSE_WRITER_H

#include "component.h"
#include <deque>
#include <string>
#include <fmt/format.h>
#include <fmt/ranges.h>
#include <cache_interface.h>
/**
 * TODO finish the clause writter
 * disc:    clause_writer, generate traffic for write clause data to l3 cache.
 * input:   every time in the clause process, decide to change the literal location
 *          need to push the current clause back to l3 cache.
 * 
 * output:  push the write requst, the addr is ignored. 
 * 
*/
class clause_writer : public componet
{
    using req_ptr = std::unique_ptr<cache_interface_req>;
    using req_ptr_q = std::deque<req_ptr>;
    using req_ptr_q_vec = std::vector<req_ptr_q>;

public:
    virtual std::string get_internal_size() const override
    {
        return fmt::format("name {} in {} out {}", "clause_writer", in.size(), out.size());
    }
    virtual std::string get_line_trace() const override
    {
        return fmt::format("name {} busy {} one {} two {}", "clause_writer", get_busy_percent(), write_one, write_two);
    }
    virtual bool empty() const override
    {
        return in.empty() and out.empty();
    }

    clause_writer(uint64_t &ct) : componet(ct) {}
    std::deque<req_ptr> in;
    std::deque<req_ptr> out;

private:
    uint64_t write_one = 0;
    uint64_t write_two = 0;

protected:
    virtual bool do_cycle() override;
};
#endif