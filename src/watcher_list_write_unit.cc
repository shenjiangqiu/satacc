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
        //busy = true;
        //add current and the other to the map;
        auto req = std::move(in_request.front());
        in_request.pop_front();
        auto as = req->as;
        auto watcherId = req->watcherId;
        req->type = ReadType::writeWatcherList;
        unsigned watcehrSize = as->get_watcher_size();
        //it's the first one, we count in_
        if (watcherId == 0)
        {
            //is the first, we only count the first, for the simplicity;
            //to calculate how many access we should have.
            busy = true;

            for (unsigned i = 0; i < watcehrSize; i++)
            {
                //push and evict the map entry;
                if (as->get_is_push_to_other(i))
                {
                    //push to other
                    auto other = as->get_pushed(i);

                    this->other_map[other]++;
                    if (other_map[other] == 1)
                    {
                        //new one
                        other_evict_queue.push_back(other);
                        if (this->other_map.size() > max_other_size)
                        {
                            auto the_last_one = other_evict_queue.front();
                            other_evict_queue.pop_front();
                            req->type = ReadType::writeWatcherList; //error here, but currently we don't need to know the address

                            out_mem_requst.push_back(copy_unit_ptr(req)); // need to find what to push back;
                            //evict the last one
                            auto size = other_map[the_last_one];
                            assert(size >= 1);
                            //to do. collect the histogram for merge
                            other_map.erase(the_last_one);
                            evict_size_histo[size - 1]++;
                        }
                    }
                    else if (this->other_map[other] >= 8)
                    {
                        //evict this
                        req->type = ReadType::writeWatcherList;
                        out_mem_requst.push_back(copy_unit_ptr(req));
                        this->other_map.erase(other); //
                        evict_size_histo[7]++;
                    }
                }
                else
                {
                    //todo implement to merge operation
                    //push to current watcher list
                    auto current_assign = as->get_value();
                    current_map[current_assign]++;
                    //evict the current size;
                    if (current_map[current_assign] >= 8)
                    {
                        current_map.erase(current_assign);
                        out_mem_requst.push_back(copy_unit_ptr(req));
                        evict_current_size_histo[7]++;
                    }
                    if (watcehrSize == 1)
                    {
                        //the first one is also the last one
                        out_mem_requst.push_back(copy_unit_ptr(req));
                        current_map.erase(current_assign);
                    }
                }
            }
        }
        else if (watcherId == watcehrSize - 1) //evict the current cache //this is the last one!
        {
            auto current_assign = as->get_value();
            //assert(current_map[current_assign] > 0);
            if (current_map.find(current_assign) != current_map.end())
            {
                busy = true;
                auto size = current_map[current_assign];
                assert(size >= 0);

                current_map.erase(current_assign);
                out_mem_requst.push_back(copy_unit_ptr(req));
                evict_current_size_histo[size - 1]++;
            }
        }
        else //other index shouldn't be here
        {
            throw;
        }
    }
    return busy;
}