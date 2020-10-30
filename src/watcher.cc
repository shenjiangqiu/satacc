#include "watcher.h"
#include <ptr_copy.hpp>
watcher::watcher(uint64_t &t) : componet(t)
{
    m_id=global_id++;
}
int watcher::global_id=0;
watcher::~watcher()
{
}
namespace sjq
{
    extern bool inside_busy;
}

bool watcher::do_cycle()
{
    bool busy = false;
    busy |= from_value_to_out_mem();
    busy |= from_process_to_out();
    busy |= from_read_watcher_to_mem();
    busy |= from_in_to_read();
    busy |= from_resp_to_insider();

    return busy;
}

bool watcher::from_value_to_out_mem()
{
    bool busy = false;
    if (!waiting_value_watcher_queue.empty())
    {
        busy = true;
        auto total_size = (unsigned)waiting_value_watcher_queue.front()->as->get_watcher_size();
        assert(total_size != 0);
        auto &current_size = waiting_value_watcher_queue.front()->watcherId;

        //FIXME
        out_memory_read_queue.push_back(copy_unit_ptr(waiting_value_watcher_queue.front()));
        out_memory_read_queue.back()->type = AccessType::ReadWatcherValue;

        current_size += 1;
        if (current_size >= total_size)
        {
            waiting_value_watcher_queue.pop_front();
        }
    }
    return busy;
}
/**
 * change log:
 * add write traffic for writing watcher list
 */
bool watcher::from_process_to_out()
{
    bool busy = false;

    if (!waiting_process_queue.empty())
    {
        auto total_size = (unsigned)waiting_process_queue.front()->as->get_watcher_size();
        assert(total_size > 0);
        auto &req = waiting_process_queue.front();
        auto &current_size = waiting_process_queue.front()->watcherId;
        busy = true;

        if (current_size == 0)
        {
            //the first one, generate the new request
            //FIXME
            out_write_watcher_list_queue.push_back(copy_unit_ptr(req));
        }
        else if (current_size == total_size - 1)
        {
            //FIXME
            out_write_watcher_list_queue.push_back(copy_unit_ptr(req));
            //the last one, generate the clean task
        }

        if (req->as->is_read_clause(current_size))
        {
            //FIXME
            out_send_queue.push_back(copy_unit_ptr(waiting_process_queue.front()));
            out_send_queue.back()->clauseId = 0;
        }
        current_size += 1;
        if (current_size >= total_size)
        {
            waiting_process_queue.pop_front();
        }
    }
    return busy;
}
bool watcher::from_read_watcher_to_mem()
{
    bool busy = false;

    if (!waiting_read_watcher_queue.empty())
    {

        auto total_size = (unsigned)waiting_read_watcher_queue.front()->as->get_watcher_size();
        auto &current_size = waiting_read_watcher_queue.front()->watcherId;
        if (current_size >= total_size) //in case the total size is 0
        {
            waiting_read_watcher_queue.pop_front();
            sjq::inside_busy = false; //finished this task
        }
        else
        {
            busy = true;

            //FIXME
            out_memory_read_queue.push_back(copy_unit_ptr(waiting_read_watcher_queue.front()));
            out_memory_read_queue.back()->type = AccessType::ReadWatcherData;
            current_size += 16;
            if (current_size >= total_size) //in case the total size is 0
            {
                waiting_read_watcher_queue.pop_front();
            }
        }
    }
    return busy;
}
bool watcher::from_in_to_read()
{
    bool busy = false;

    if (!in_task_queue.empty() and waiting_read_watcher_queue.size() < read_size)
    {
        busy = true;
        //FIXME
        waiting_read_watcher_queue.push_back(std::move(in_task_queue.front()));
        waiting_read_watcher_queue.back()->watcherId = 0;
        assert(waiting_read_watcher_queue.back()->as != nullptr);
        in_task_queue.pop_front();
    }
    return busy;
}
bool watcher::from_resp_to_insider()
{
    bool busy = false;

    if (!in_memory_resp_queue.empty())
    {
        busy = true;

        auto &type = in_memory_resp_queue.front()->type;
        auto &index = in_memory_resp_queue.front()->watcherId;
        auto &as = in_memory_resp_queue.front()->as;
        //into waiting_value_watcher_queue
        if (type == AccessType::ReadWatcherData and waiting_value_watcher_queue.size() < value_size)
        {
            if (index + 16 >= (unsigned)(as->get_watcher_size())) //the last one
            {
                //FIXME
                waiting_value_watcher_queue.push_back(std::move(in_memory_resp_queue.front()));
                waiting_value_watcher_queue.back()->watcherId = 0; //reset to zero
                //
            }
            in_memory_resp_queue.pop_front();
        }
        //into waiting_process_queue
        else if (type == AccessType::ReadWatcherValue and waiting_process_queue.size() < process_size)
        {
            if (index + 1 >= (unsigned)(as->get_watcher_size())) //the last one
            {
                waiting_process_queue.push_back(std::move(in_memory_resp_queue.front()));
                waiting_process_queue.back()->watcherId = 0;
                //
            }
            in_memory_resp_queue.pop_front();
        }
    }
    return busy;
}