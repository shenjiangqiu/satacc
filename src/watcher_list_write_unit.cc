#include "watcher_list_write_unit.h"
#include <ptr_copy.hpp>
watcher_list_write_unit::~watcher_list_write_unit()
{
}
/**
 * tasks:   1, when recieve the first watcher, add them to current map and other map
 *              1.1 when the buffer is full, generate the write request to l3
 *              1.2 when the buffer entry is full, generate the write request to l3
 *          2, when recieve the last watcher, delete them from current map if exist
 * 
 */
bool watcher_list_write_unit::do_cycle()
{
    //TODO it's very difficult to do it!
    bool busy = false;
    if (!in_request.empty())
    {
        busy = true;
        //busy = true;
        //add current and the other to the map;
        auto req = std::move(in_request.front());
        in_request.pop_front();
        assert(req);
        auto as = req->as;

        req->type = AccessType::WriteWatcherList;
        unsigned watcehrSize = as->get_watcher_size();
        assert(watcehrSize != 0);
        auto value = (as->get_is_push_to_other(req->watcherId) ? as->get_pushed(req->watcherId) : as->get_value());
        //assert(m_id == value % total_size);
        //assert(m_id == value % total_size);

        if (current_map.count(value))
        {
            if (current_map[value] < max_merge)
            {
                current_map[value]++;
            }
            else
            {
                out_mem_requst.push_back(std::move(req));
                total_origin_package += current_map.at(value);
                total_merged_package++;
                current_map.erase(value);
                current_req.erase(value);
            }
        }
        else
        {
            //add this new entry
            if (current_map.size() >= max_entry)
            { //need to evict one entry;
                while (!current_req.count(entry_aging_queue.front()))
                {
                    entry_aging_queue.pop();
                }
                auto oldest_entry = entry_aging_queue.front();

                entry_aging_queue.pop();
                auto &evict = current_req.at(oldest_entry);
                assert(evict);
                out_mem_requst.push_back(std::move(evict));
                total_origin_package += current_map.at(oldest_entry);
                total_merged_package++;
                current_req.erase(oldest_entry);
                current_map.erase(oldest_entry);
            }
            //add this new entry
            entry_aging_queue.push(value);

            current_map.insert({value, 1});
            assert(req);
            current_req.insert({value, std::move(req)});
        }
    }
    return busy;
}