#include "clause_writer.h"
#include <ptr_copy.hpp>
bool clause_writer::do_cycle()
{
    bool busy = false;
    if (!in.empty())
    {
        busy = true;

        //in will be here because a clause is finished processing.
        auto &req = in.front();
        req->type = ReadType::writeClause;

        auto watcher_index = req->watcherId;
        assert(watcher_index < (unsigned)req->as->get_watcher_size());
        auto clause_size = req->as->get_clause_detail(watcher_index).size();
        //should be the last one
        assert(clause_size == req->clauseId + 1);
        req->type = ReadType::writeClause;
        //FIXME copy or move
        if (clause_size <= 16)
        {
            out.push_back(std::move(req));
            write_one++;
        }
        else
        {
            //else we assume this request will touch two cache line(will swap the first and the middle one)
            out.push_back(copy_unit_ptr(req));
            out.push_back(std::move(req));
            write_two++;
        }

        in.pop_front();
    }

    return busy;
}