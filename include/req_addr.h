#ifndef REQ_ADDR_H
#define REQ_ADDR_H
#include <cache_interface.h>
#include <mem_req_interface.h>
#include <memory>

uint64_t get_addr_by_req(const std::unique_ptr<cache_interface_req> &req)
{
    uint64_t addr;
    auto type = req->type;
    auto as = req->as;
    auto watcherId = req->watcherId;
    auto clauseId = req->clauseId;
    switch (type)
    {
    case ReadType::ReadClauseData:
        /* code */

        addr = as->get_clause_addr(watcherId);
        addr += clauseId * 4;

        break;
    case ReadType::ReadClauseValue:
        addr = as->get_clause_detail(watcherId)[clauseId];
        break;
    case ReadType::ReadWatcher:
        assert(as != nullptr);
        addr = as->get_addr();
        addr += 4 * watcherId;
        break;
    case ReadType::WatcherReadValue:
        assert(as->get_watcher_size() != 0);
        addr = as->get_block_addr(watcherId);

        break;
    case ReadType::writeClause:
        addr = as->get_clause_addr(watcherId);
        break;
    case ReadType::writeWatcherList:
        addr = as->get_pushed_watcher_list_tail_addr(watcherId);
        break;
    default:
        throw;
        break;
    }
    return addr;
}
#endif