#include "clause.h"
#include "cache_interface.h"
#include <ptr_copy.hpp>
clause::clause(uint64_t &t) : componet(t)
{
}

clause::~clause()
{
}

bool clause::do_cycle()
{

    bool busy = false;
    busy |= process_waiting_to_out();
    busy |= value_waiting_to_mem_out();
    busy |= data_waiting_to_mem_out();
    busy |= mem_in_to_comp();
    busy |= task_to_data_waiting();
    return busy;
}

bool clause::task_to_data_waiting() //get the task and send to data waiting queue
{
    bool busy = false;
    if (!in_task_queue.empty() and clause_data_read_waiting_queue.size() < data_size)
    {
        busy = true;
        //FIXME should be error here
        clause_data_read_waiting_queue.push_back(std::move(in_task_queue.front()));
        clause_data_read_waiting_queue.back()->type = AccessType::ReadClauseData;
        clause_data_read_waiting_queue.back()->clauseId = 0;
        in_task_queue.pop_front();
    }
    return busy;
}
bool clause::data_waiting_to_mem_out() // get from data_waiting queue, and get clause detail
{
    bool busy = false;
    if (!clause_data_read_waiting_queue.empty())
    {
        busy = true;

        auto &index = clause_data_read_waiting_queue.front()->watcherId;
        auto &clauseId = clause_data_read_waiting_queue.front()->clauseId;
        auto total_clause_size = clause_data_read_waiting_queue.front()->as->get_clause_literal(index).size();
        //error FIXME
        //should copy here,not move
        //auto copy = std::unique_ptr<cache_interface_req>(new cache_interface_req(*clause_data_read_waiting_queue.front()));
        auto copy = copy_unit_ptr(clause_data_read_waiting_queue.front());
        out_memory_read_queue.push_back(std::move(copy));
        out_memory_read_queue.back()->type = AccessType::ReadClauseData;
        clauseId += 16;
        if (clauseId >= total_clause_size)
        {
            clause_data_read_waiting_queue.pop_front();
        }
    }
    return busy;
}
bool clause::value_waiting_to_mem_out() // get from value_waiting, and get value details
{
    bool busy = false;
    if (!clause_value_read_waiting_queue.empty())
    {
        busy = true;

        auto &req = clause_value_read_waiting_queue.front();
        auto &index = req->watcherId;
        auto total_clasue_size = req->as->get_clause_literal(index).size();
        auto &clauseId = req->clauseId;

        out_memory_read_queue.push_back(copy_unit_ptr(req));
        out_memory_read_queue.back()->type = AccessType::ReadClauseValue;
        clauseId += 1;

        if (clauseId >= total_clasue_size)
        {
            clause_value_read_waiting_queue.pop_front();
        }
    }
    return busy;
}
bool clause::mem_in_to_comp() //to value_waiting and process waiting
{
    bool busy = false;
    if (!in_memory_resp_queue.empty())
    {
        auto &req = in_memory_resp_queue.front();
        auto type = req->type;
        auto watcherId = req->watcherId;
        auto clauseId = req->clauseId;
        auto ass = req->as;
        auto total_clause_size = ass->get_clause_literal(watcherId).size();
        if (type == AccessType::ReadClauseData and
            clause_value_read_waiting_queue.size() < data_size)
        {
            busy = true;

            if (clauseId + 16 >= total_clause_size)
            {
                //the last one
                auto &new_req = req;
                new_req->type = AccessType::ReadClauseValue;
                new_req->clauseId = 0;
                clause_value_read_waiting_queue.push_back(std::move(new_req));
            }
            in_memory_resp_queue.pop_front();
        }
        else if (clause_process_waiting_queue.size() < process_size)
        {
            busy = true;

            //type is value read
            if (clauseId + 1 >= total_clause_size) //the last one
            {
                auto &new_req = req;
                new_req->clauseId = 0;
                clause_process_waiting_queue.push_back(std::move(new_req));
            }
            in_memory_resp_queue.pop_front();
        }
    }
    return busy;
}

bool clause::process_waiting_to_out() //process the clause and send out
{
    bool busy = false;
    if (!clause_process_waiting_queue.empty())
    {
        busy = true;

        auto &req = clause_process_waiting_queue.front();
        //auto type = req.type;
        auto watcherId = req->watcherId;
        auto &clauseId = req->clauseId;
        auto ass = req->as;
        auto total_clause_size = ass->get_clause_literal(watcherId).size();

        clauseId += 1;

        if (clauseId >= total_clause_size) //the last one
        {
            auto new_req = copy_unit_ptr(req);
            new_req->clauseId = 0;
            if (ass->get_generated(watcherId) != nullptr)
            {
                new_req->as = ass->get_generated(watcherId);
                assert(new_req->as != nullptr);
                new_req->watcherId = 0;
                new_req->clauseId = 0;
                new_req->type = AccessType::ReadWatcherData;
                assert(new_req);

                out_queue.push_back(std::move(new_req));
            }
            //push
            //process the write clause traffic
            busy = true;
            req->clauseId--;
            assert(req);
            out_clause_write_queue.push_back(std::move(req));
            assert(out_clause_write_queue.back()->clauseId + 1 == out_clause_write_queue.back()->as->get_clause_detail(watcherId).size());

            clause_process_waiting_queue.pop_front();
        }
        //else just continue, we are processing...;

        /* code */
    }
    return busy;
}