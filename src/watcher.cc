#include "watcher.h"

watcher::watcher(uint64_t &t) : componet(t)
{
}

watcher::~watcher()
{
}

bool watcher::cycle()
{
    bool busy = false;

    //from waiting_value_watcher_queue to out_memory_read_queue,
    //notice: each time only read one value, because they are not continued
    /*notice: waiting_value_watcher_queue only contain each assign once,so when
    the memory return, only if we recieve the last package, we push to the 
    waiting_value_watcher_queue*/
    if (!waiting_value_watcher_queue.empty())
    {
        busy = true;
        auto total_size = (unsigned)waiting_value_watcher_queue.front().as->get_watcher_size();
        auto &current_size = waiting_value_watcher_queue.front().watcherId;

        out_memory_read_queue.push_back(waiting_value_watcher_queue.front());
        out_memory_read_queue.back().type = ReadType::WatcherReadValue;

        current_size += 1;
        if (current_size >= total_size)
        {
            waiting_value_watcher_queue.pop_front();
        }
    }
    //from waiting_process_queue to out_send_queue
    if (!waiting_process_queue.empty())
    {
        auto total_size = (unsigned)waiting_process_queue.front().as->get_watcher_size();
        auto &current_size = waiting_process_queue.front().watcherId;
        busy = true;

        if (waiting_process_queue.front().as->is_read_clause(current_size))
        {
            out_send_queue.push_back(waiting_process_queue.front());
            out_send_queue.back().clauseId = 0;
        }
        current_size += 1;
        if (current_size >= total_size)
        {
            waiting_process_queue.pop_front();
        }
    }

    //from waiting_read_watcher_queue to out_memory_read_queue
    //notice: we push 64 bytes request per cycle
    if (!waiting_read_watcher_queue.empty())
    {
        busy = true;

        auto total_size = (unsigned)waiting_read_watcher_queue.front().as->get_watcher_size();
        auto &current_size = waiting_read_watcher_queue.front().watcherId;

        out_memory_read_queue.push_back(waiting_read_watcher_queue.front());
        out_memory_read_queue.back().type = ReadType::ReadWatcher;
        current_size += 16;
        if (current_size >= total_size)
        {
            waiting_read_watcher_queue.pop_front();
        }
    }

    //from in_task to waiting_read_watcher_queue
    if (!in_task_queue.empty() and waiting_read_watcher_queue.size() < read_size)
    {
        busy = true;

        waiting_read_watcher_queue.push_back(in_task_queue.front());
        waiting_read_watcher_queue.back().watcherId = 0;
        in_task_queue.pop_front();
    }

    //from in_memory_resp_queuue to inside queue

    if (!in_memory_resp_queue.empty())
    {
        busy = true;

        auto &type = in_memory_resp_queue.front().type;
        auto &index = in_memory_resp_queue.front().watcherId;
        auto &as = in_memory_resp_queue.front().as;
        //into waiting_value_watcher_queue
        if (type == ReadType::ReadWatcher and waiting_value_watcher_queue.size() < value_size)
        {
            if (index + 16 >= (unsigned)(as->get_watcher_size())) //the last one
            {
                waiting_value_watcher_queue.push_back(in_memory_resp_queue.front());
                waiting_value_watcher_queue.back().watcherId = 0; //reset to zero
                //
            }
            in_memory_resp_queue.pop_front();
        }
        //into waiting_process_queue
        else if (type == ReadType::WatcherReadValue and waiting_process_queue.size() < process_size)
        {
            if (index + 1 >= (unsigned)(as->get_watcher_size())) //the last one
            {
                waiting_process_queue.push_back(in_memory_resp_queue.front());
                waiting_process_queue.back().watcherId = 0;
                //
            }
            in_memory_resp_queue.pop_front();
        }
    }
    if (busy)
        this->busy++;
    else
        this->idle++;
    return busy;
}