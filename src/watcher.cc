#include "watcher.h"


#include <ptr_copy.hpp>
watcher::watcher(uint64_t &t) : componet(t) { m_id = global_id++; }
int watcher::global_id = 0;
watcher::~watcher() {}
namespace sjq {
extern bool inside_busy;
}

bool watcher::do_cycle() {
  bool busy = false;
  busy |= from_watcher_meta_to_memory();
  busy |= from_value_to_out_mem();
  busy |= from_process_to_out();
  busy |= from_read_watcher_to_mem();
  busy |= from_in_to_read();
  busy |= from_resp_to_insider();
  if (!busy) {
    // the reason of ideal
    total_idle++;
    if (outgoing_read_watcher_data) {
      total_idle_no_watcher_data++;
    } else if (outgoing_read_watcher_value) {
      total_idle_no_value++;
    } else {
      assert(in_task_queue.empty());
      total_idle_no_task++;
    }
  } else {
    total_busy++;
  }

  //update the average.
  //FIXME there is a bug
  update(average_inflight,num_inflight_request,current_cycle);

  return busy;
}

bool watcher::from_value_to_out_mem() {
  bool busy = false;
  if (!waiting_value_watcher_queue.empty()) {
    busy = true;
    auto total_size =
        (unsigned)waiting_value_watcher_queue.front()->as->get_watcher_size();
    assert(total_size != 0);
    auto &current_size = waiting_value_watcher_queue.front()->watcherId;

    out_memory_read_queue.push_back(
        copy_unit_ptr(waiting_value_watcher_queue.front()));
    out_memory_read_queue.back()->type = AccessType::ReadWatcherValue;
    outgoing_read_watcher_value++;
    current_size += 1;
    if (current_size >= total_size) {
      waiting_value_watcher_queue.pop_front();
    }
  }
  return busy;
}
/**
 * change log:
 * add write traffic for writing watcher list
 */
bool watcher::from_process_to_out() {
  bool busy = false;

  if (!waiting_process_queue.empty()) {
    auto total_size =
        (unsigned)waiting_process_queue.front()->as->get_watcher_size();
    assert(total_size > 0);
    auto &req = waiting_process_queue.front();
    auto &current_size = waiting_process_queue.front()->watcherId;
    busy = true;

    // the first one, generate the new request
    assert(req);
    out_write_watcher_list_queue.push_back(copy_unit_ptr(req));

    if (req->as->is_read_clause(current_size)) {

      out_send_queue.push_back(copy_unit_ptr(waiting_process_queue.front()));
      out_send_queue.back()->clauseId = 0;
    }
    current_size += 1;
    if (current_size >= total_size) {
      //pop a watcher list
      num_inflight_request--;

      assert((long long)num_inflight_request>=0);
      waiting_process_queue.pop_front();
    }
  }
  return busy;
}
bool watcher::from_read_watcher_to_mem() {
  bool busy = false;

  if (!waiting_read_watcher_queue.empty()) {

    auto total_size =
        (unsigned)waiting_read_watcher_queue.front()->as->get_watcher_size();
    auto &current_size = waiting_read_watcher_queue.front()->watcherId;
    if (current_size >= total_size) // in case the total size is 0
    {
      waiting_read_watcher_queue.pop_front();
      num_inflight_request--;
      assert(num_inflight_request>=0);
      sjq::inside_busy = false; // finished this task
    } else {
      busy = true;

      out_memory_read_queue.push_back(
          copy_unit_ptr(waiting_read_watcher_queue.front()));
      out_memory_read_queue.back()->type = AccessType::ReadWatcherData;
      out_memory_read_queue.back()->addr =
          out_memory_read_queue.back()->as->get_addr() + (current_size * 4);
      outgoing_read_watcher_data++;

      current_size += 16;
      if (current_size >= total_size) //the end of the request has been sent
      {
        waiting_read_watcher_queue.pop_front();
      }
    }
  }
  return busy;
}
// need add a new pipeline that access the metadata.
bool watcher::from_in_to_read() {
  // in: the new litreal, we need to get the watcher list addr. and then to get
  // the watchers inside the watcher list.
  bool busy = false;
  if (!in_task_queue.empty()) {
    num_inflight_request++;

    auto &req = in_task_queue.front();
    req->type = AccessType::ReadWatcherMetaData;

    waiting_read_meta_queue.push_back(std::move(req));
    in_task_queue.pop_front();
    busy = true;
  }
  /*
  if (!in_task_queue.empty() and waiting_read_watcher_queue.size() < read_size)
  {
      busy = true;

      waiting_read_watcher_queue.push_back(std::move(in_task_queue.front()));
      waiting_read_watcher_queue.back()->watcherId = 0;
      assert(waiting_read_watcher_queue.back()->as != nullptr);
      in_task_queue.pop_front();
  }
  */
  return busy;
}
bool watcher::from_watcher_meta_to_memory() {
  bool busy = false;
  if (!waiting_read_meta_queue.empty()) {
    auto &req = waiting_read_meta_queue.front();
    req->type = AccessType::ReadWatcherMetaData;
    req->watcherId = 0;
    out_memory_read_queue.push_back(std::move(req));
    waiting_read_meta_queue.pop_front();
    busy = true;
  }

  return busy;
}
bool watcher::from_resp_to_insider() {
  bool busy = false;

  if (!in_memory_resp_queue.empty()) {
    busy = true;
    auto &req = in_memory_resp_queue.front();
    auto &type = req->type;
    auto &index = req->watcherId;
    auto &as = req->as;
    // into waiting_value_watcher_queue
    if (type == AccessType::ReadWatcherData) {
      outgoing_read_watcher_data--;
      assert((int)outgoing_read_watcher_data >= 0);
      if (index + 16 >= (unsigned)(as->get_watcher_size())) // the last one
      {

        waiting_value_watcher_queue.push_back(std::move(req));
        waiting_value_watcher_queue.back()->watcherId = 0; // reset to zero
        //
      }
      // for others , just delete it.
      in_memory_resp_queue.pop_front();
    }
    // into waiting_process_queue
    else if (type == AccessType::ReadWatcherValue) {
      outgoing_read_watcher_value--;
      assert((int)outgoing_read_watcher_value >= 0);
      if (index + 1 >= (unsigned)(as->get_watcher_size())) // the last one
      {
        waiting_process_queue.push_back(std::move(req));
        waiting_process_queue.back()->watcherId = 0;
        //
      }
      in_memory_resp_queue.pop_front();
    } else if (type == AccessType::ReadWatcherMetaData) {
      busy = true;

      req->watcherId = 0;
      req->type = AccessType::ReadWatcherData;
      waiting_read_watcher_queue.push_back(std::move(req));

      assert(waiting_read_watcher_queue.back()->as != nullptr);
      in_memory_resp_queue.pop_front();
    } else {
      throw;
    }
  }
  return busy;
}
