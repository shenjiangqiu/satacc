#include "acc.h"
#include <mem_req_interface.h>

#include "boost/program_options.hpp"
#include "fmt/format.h"
#include <addr_utiles.h>
#include <fstream>
#include <memreq_info.h>
#include <new_intersim_wrapper.h>
#include <req_addr.h>
#include <rusttools.h>
#include <sstream>
namespace po = boost::program_options;

namespace sjq {
bool inside_busy;
} // namespace sjq

void acc::init_watcher_and_clause() {
  // init the components
  for (unsigned i = 0; i < num_watchers; i++) {
    auto new_watcher = new watcher(current_cycle);
    watchers.push_back(new_watcher);
    m_componets.push_back(new_watcher);

    auto m_private_cache =
        new private_cache(8, private_cache_size, current_cycle);
    m_componets.push_back(m_private_cache);
    m_private_caches.push_back(m_private_cache);
  }
  for (unsigned i = 0; i < num_clauses; i++) {
    auto new_clause = new clause(current_cycle);
    clauses.push_back(new_clause);
    m_componets.push_back(new_clause);
  }
}

// TODO, currently , watcher to clause need to be change
void acc::add_hook_from_watcher_out_actions() {
  // handle from watchers to clauses, and from watchers to memory
  for (unsigned i = 0; i < num_watchers; i++) {
    // the pass for 1, watcher to clause, 2, wathcer and clause to memory. Do
    // not include the internal cycle()
    clock_passes.push_back([i, this]() -> bool {
      bool busy = false;
      // static int ii = 0; //current cycle's choice

      // int total_c_p_w = num_clauses / num_watchers;
      // int clause_id = watcher_id * total_c_p_w +
      // watchers[watcher_id]->next_c(total_c_p_w); bug here , ii will be shared
      // across all the lambda!!!!! ii = (ii + 1) % (num_clauses /
      // num_watchers); send the request to clause unit
      if (!watchers.at(i)->out_send_queue.empty() and
          watcher_to_clause_icnt->in_reqs[i].size() < 64) {
        busy = true;
        auto &req = watchers[i]->out_send_queue.front();
        req->icnt_from = i;
        if (this->rust_config.watcher_to_clause_type ==
            sjqrusttools::WatcherToClauseType::Icnt) {

          req->icnt_to = req->as->get_clause_id(req->watcherId) % num_watchers;
        } else {
          assert(rust_config.watcher_to_clause_type ==
                 sjqrusttools::WatcherToClauseType::Streight);
          req->icnt_to = req->icnt_from;
        }
        if (req->icnt_to == req->icnt_from) {
          watcher_to_clause_icnt->out_reqs[i].push_back(std::move(req));
        } else {
          total_watcher_clause_icnt_traffic += 8;
          req->m_size = 8;
          watcher_to_clause_icnt->in_reqs[i].push_back(std::move(req));
        }

        watchers[i]->out_send_queue.pop_front();
      }

      // send the memory request to cache interfase //it's direct to l3 cache
      if (!watchers[i]->out_memory_read_queue.empty()) {
        if (watchers[i]->out_memory_read_queue.front()->type ==
            AccessType::ReadWatcherData) {
          auto source = i;
          if (memory_read_icnt->has_buffer(source)) {
            busy = true;
            // assert(watchers[watcher_id]->out_memory_read_queue.front()->as !=
            // nullptr);
            auto &req = watchers[i]->out_memory_read_queue.front();

            assert(req->type == AccessType::ReadWatcherData);
            // error here
            // m_cache_interface->in_request_queue.push_back(std::move(req));
            // m_cache_interface->in_request_queue.back()->ComponentId =
            // watcher_id; now push to icnt
            req->ComponentId = i;
#ifdef SJQ_ICNT_DEBUG
            std::cout << "from watcher to icnt: " << i << std::endl;
            std::cout << *req << std::endl;

#endif
            req->icnt_from = source;
            req->icnt_to = num_watchers +
                           get_partition_id_by_addr(req->addr, num_partition);
            req->m_size = 12;
            // total_memory_icnt_traffic+=64;
            memory_read_icnt->in_reqs[source].push_back(std::move(req));

            watchers[i]->out_memory_read_queue.pop_front();
          }
        }
        // send private cache to cache//to private cache!!
        else if (watchers[i]->out_memory_read_queue.front()->type ==
                 AccessType::ReadWatcherMetaData) {
          auto source = i;
          if (memory_read_icnt->has_buffer(source)) {
            busy = true;
            // assert(watchers[watcher_id]->out_memory_read_queue.front()->as !=
            // nullptr);
            auto &req = watchers[i]->out_memory_read_queue.front();

            assert(req->type == AccessType::ReadWatcherMetaData);
            // error here
            // m_cache_interface->in_request_queue.push_back(std::move(req));
            // m_cache_interface->in_request_queue.back()->ComponentId =
            // watcher_id; now push to icnt
            req->ComponentId = i;
#ifdef SJQ_ICNT_DEBUG
            std::cout << "from watcher to icnt: " << i << std::endl;
            std::cout << *req << std::endl;

#endif
            req->icnt_from = source;
            req->icnt_to =
                num_watchers +
                get_partition_id_by_addr(req->as->get_watcher_list_meta_addr(),
                                         num_partition);
            req->m_size = 12;
            memory_read_icnt->in_reqs[source].push_back(std::move(req));

            watchers[i]->out_memory_read_queue.pop_front();
          }
        } else {
          // assert(watchers[watcher_id]->out_memory_read_queue.front()->type ==
          // AccessType::ReadWatcherValue);
          if (!watchers[i]->out_memory_read_queue.empty() and
              m_private_caches[i]->recieve_rdy()) {
            busy = true;
            assert(watchers[i]->out_memory_read_queue.front()->as != nullptr);
            auto &req = watchers[i]->out_memory_read_queue.front();
            req->ComponentId = i;
            assert(req->type == AccessType::ReadWatcherValue);
#ifdef SJQ_ICNT_DEBUG
            std::cout << "from watcher to private cache: " << i << std::endl;
            std::cout << *req << std::endl;

#endif
            m_private_caches[i]->in_request.push_back(std::move(req));

            watchers[i]->out_memory_read_queue.pop_front();
          }
        }
      }
      // send private cache to l3 cache
      if (!m_private_caches[i]->out_miss_queue.empty()) {
        auto &req = m_private_caches[i]->out_miss_queue.front();
        if (memory_read_icnt->has_buffer(i)) {
          busy = true;
          // req->ComponentId = i;
#ifdef SJQ_ICNT_DEBUG
          std::cout << "from private cache to icnt: " << i << std::endl;
          std::cout << *req << std::endl;

#endif
          req->icnt_from = i;
          req->icnt_to =
              num_watchers + get_partition_id_by_addr(req->addr, num_partition);
          req->m_size = 12;
          memory_read_icnt->in_reqs[i].push_back(std::move(req));
          m_private_caches[i]->out_miss_queue.pop_front();
        }
      }
      return busy;
    });
  }
}

void acc::add_hook_from_clause_to_mem() {
  // send clause mem request
  for (unsigned i = 0; i < num_clauses; i++) {
    int clauseId = i;
    unsigned n_w = num_watchers;
    unsigned n_c = num_clauses;
    clock_passes.push_back([clauseId, n_w, n_c, this]() {
      bool busy = false;
      if (!clauses[clauseId]->is_out_memory_read_queue_empty()) {
        if (clauses[clauseId]->get_from_out_memory_read_queue()->type ==
            AccessType::ReadClauseData) {
          auto source = clauseId / (n_c / n_w);
          if (memory_read_icnt->has_buffer(source)) {
            busy = true;
            assert(clauses[clauseId]->get_from_out_memory_read_queue()->as !=
                   nullptr);
            auto &req = clauses[clauseId]->get_from_out_memory_read_queue();
            req->ComponentId = clauseId + num_watchers;
#ifdef SJQ_ICNT_DEBUG
            std::cout << fmt::format("from clause to icnt:{} {}", clauseId,
                                     source)
                      << std::endl;
            // std::cout << "from clause to icnt: " << i << std::endl;
            std::cout << *req << std::endl;

#endif
            req->icnt_from = source;

            req->icnt_to =
                num_watchers +
                get_partition_id_by_addr(get_addr_by_req(req), num_partition);
            req->m_size = 12;

            total_memory_icnt_traffic += 64;

            memory_read_icnt->in_reqs[source].push_back(std::move(req));
            clauses[clauseId]->pop_from_out_memory_read_queue();
          }
        } else {
          // push value request to private cache
          auto &req = clauses[clauseId]->get_from_out_memory_read_queue();
          assert(req->type == AccessType::ReadClauseValue);
          auto watcherId = clauseId / (num_clauses / num_watchers);
          if (m_private_caches[watcherId]->recieve_rdy()) {
            busy = true;
            assert(req->as != nullptr);
            req->ComponentId = clauseId + num_watchers;
#ifdef SJQ_ICNT_DEBUG
            // std::cout << "from clause to private cache" << std::endl;
            std::cout << fmt::format("from clause to private cache:{} {}",
                                     clauseId, watcherId)
                      << std::endl;

            std::cout << *req << std::endl;

#endif
            m_private_caches[watcherId]->in_request.push_back(
                std::move(clauses[clauseId]->get_from_out_memory_read_queue()));

            clauses[clauseId]->pop_from_out_memory_read_queue();
          }
        }
      }

      return busy;
    });
  } // end for clause
}

void acc::add_hook_from_cache_to_clause_and_watchers() {

  // add pass to send cache response to clauses and watchers
  clock_passes.push_back([this]() {
    bool busy = false;
    for (auto i = 0u; i < num_partition; i++) {
      if (!m_cache_interface->out_resp_queues[i].empty()) {
        auto &req = m_cache_interface->out_resp_queues[i].front();
        assert(i ==
               get_partition_id_by_addr(get_addr_by_req(req), num_partition));
        req->icnt_from = i + num_watchers;
        req->icnt_to = req->ComponentId < num_watchers
                           ? req->ComponentId
                           : (req->ComponentId - num_watchers) /
                                 (num_clauses / num_watchers);
        req->m_size = 64;
        total_memory_icnt_traffic += 64;
        memory_read_icnt->in_reqs[i + num_watchers].push_back(std::move(req));
        m_cache_interface->out_resp_queues[i].pop_front();
        /*
        //this code if for icnt to watcher and clause
        if (component_id >= num_watchers)
        {
            //the case it's a clause request

            int clause_id = component_id - num_watchers;

            if (req->type == AccessType::ReadClauseData)
            { //send to cleause

                if (clauses[clause_id]->recieve_mem_rdy())
                {
                    busy = true;

                    clauses[clause_id]->in_memory_resp_queue.push_back(std::move(m_cache_interface->out_resp_queue.front()));
                    m_cache_interface->out_resp_queue.pop_front();
                }
            }
            else
            {
                //send to private cache

                assert(m_cache_interface->out_resp_queue.front()->type ==
        AccessType::ReadClauseValue); int watcherId = clause_id / (num_clauses /
        num_watchers);
                m_private_caches[watcherId]->in_resp.push_back(std::move(m_cache_interface->out_resp_queue.front()));
                m_cache_interface->out_resp_queue.pop_front();
            }
        }
        else
        {
            //the case it's a watcher request
            int watcher_id = component_id;
            if (m_cache_interface->out_resp_queue.front()->type ==
        AccessType::ReadWatcherData)
            {
                //it's a watcher data request , send to watcher
                if (watchers[watcher_id]->recieve_mem_rdy())
                {
                    busy = true;
                    watchers[watcher_id]->in_memory_resp_queue.push_back(std::move(m_cache_interface->out_resp_queue.front()));
                    m_cache_interface->out_resp_queue.pop_front();
                }
            }
            else
            {
                assert(m_cache_interface->out_resp_queue.front()->type ==
        AccessType::ReadWatcherValue);
                //int watcherId = clause_id / (num_clauses / num_watchers);
                m_private_caches[watcher_id]->in_resp.push_back(std::move(m_cache_interface->out_resp_queue.front()));
                m_cache_interface->out_resp_queue.pop_front();
            }
        }
        */
      }
    }
    return busy;
  });
}

void acc::add_hook_from_private_cache() {
  // from private cache to watchers and clauses.
  for (unsigned i = 0; i < num_watchers; i++) {
    clock_passes.push_back([i, this]() {
      bool busy = false;
      unsigned watcherId = i;
      if (!m_private_caches[watcherId]->out_send_q.empty()) {
        busy = true;
        auto &req = m_private_caches[watcherId]->out_send_q.front();
        if (req->type == AccessType::ReadClauseValue) {
          assert(req->ComponentId >= num_watchers);
          auto clauseId = req->ComponentId - num_watchers;
          assert(clauseId / (num_clauses / num_watchers) == watcherId);
#ifdef SJQ_ICNT_DEBUG
          std::cout << fmt::format("from private cache {} to clause {}", i,
                                   clauseId)
                    << std::endl;
          std::cout << *req << std::endl;
#endif
          clauses[clauseId]->push_to_in_memory_resp_queue(std::move(req));

          m_private_caches[watcherId]->out_send_q.pop_front();
        } else {
          assert(req->type == AccessType::ReadWatcherValue);
          assert(req->ComponentId == watcherId);
// std::cout << "watcher:" << watcherId << "push response from private cache" <<
// std::endl;
#ifdef SJQ_ICNT_DEBUG
          std::cout << fmt::format("from private cache {} to watcher {}", i,
                                   watcherId)
                    << std::endl;
          std::cout << *req << std::endl;
#endif
          watchers[watcherId]->in_memory_resp_queue.push_back(std::move(req));
          m_private_caches[watcherId]->out_send_q.pop_front();
        }
      }
      return busy;
    });
  }
}

void acc::add_hook_from_trail_to_watcher() {
  // add the pass for from trail to watchers

  clock_passes.push_back([this]() -> bool {
    if (enable_sequential and
        !std::all_of(m_componets.begin(), m_componets.end(),
                     [](auto &c) { return c->empty(); })) {
      return false;
    }
    bool busy = false;
    if (!in_m_trail.empty()) {
      auto &req = in_m_trail.front();
      int watcher_id = 0;
      if (single_watcher) {
        watcher_id = 0;
      } else {
        watcher_id = (req->as->get_value() / 2) % num_watchers;
      }
      if (watchers[watcher_id]->recieve_rdy()) {
        busy = true;
        assert(req->as != nullptr);
        watchers[watcher_id]->in_task_queue.push_back(std::move(req));
        sjq::inside_busy = true;
        in_m_trail.pop_front();
      }
    }
    // change to another watcher now

    return busy;
  });
}

void acc::add_hook_from_clause_to_trail() {
  // add the pass for from clauses to trail
  for (unsigned i = 0; i < num_clauses; i++) {
    clock_passes.push_back([i, this]() {
      bool busy = false;
      int clause_id = i;
      if (!clauses[clause_id]->is_out_queue_empty()) {
        busy = true;
        assert(clauses[clause_id]->get_from_out_queue()->as != nullptr);
        in_m_trail.push_back(
            std::move(clauses[clause_id]->get_from_out_queue()));
        clauses[clause_id]->pop_from_out_queue();
      }
      return busy;
    });
  }
}

void acc::add_hook_from_watcher_to_writeuite() {

  // from watcher to watcher write_unit
  for (unsigned i = 0; i < num_watchers; i++) {
    clock_passes.push_back([i, this]() {
      bool busy = false;
      if (!watchers[i]->out_write_watcher_list_queue.empty()) {
        busy = true;
        auto &req = watchers[i]->out_write_watcher_list_queue.front();
        assert(req->ComponentId == i);
        if (!req->as->get_is_push_to_other(req->watcherId)) {
          // to local writer unit
          m_watcher_write_unit[i]->in_request.push_back(std::move(req));
          watchers[i]->out_write_watcher_list_queue.pop_front();
        } else {

          // m_watcher_write_unit->in_request.push_back(std::move(req));
          req->icnt_from = i;

          req->icnt_to = req->as->get_pushed(req->watcherId) % num_watchers;

          // might also be local!!!
          if (req->icnt_from == req->icnt_to) {
            watcher_to_writer_icnt->out_reqs[i].push_back(std::move(req));
          } else {
            req->m_size = 4;
            total_watcher_writer_icnt_traffic += 8;
            watcher_to_writer_icnt->in_reqs[i].push_back(std::move(req));
          }
          watchers[i]->out_write_watcher_list_queue.pop_front();
        }
      }

      return busy;
    });
  }
}

void acc::add_hook_from_clause_to_writeuint() {

  // from clause to clause write unit
  for (unsigned i = 0; i < num_clauses; i++) {
    clock_passes.push_back([i, this]() {
      bool busy = false;
      if (!clauses[i]->is_out_clause_write_queue_empty()) {
        busy = true;
        auto &req = clauses[i]->get_from_out_clause_write_queue();
        req->ComponentId = i + num_watchers;
        auto writer_id = i / (num_clauses / num_watchers);

        m_clause_write_unit[writer_id]->in.push_back(std::move(req));
        clauses[i]->pop_from_out_clause_write_queue();
      }

      return busy;
    });
  }
}

void acc::add_hook_from_clause_write_unit_to_cache() {
  // TODO from writer to cache
  clock_passes.push_back([this]() {
    bool busy = false;
    for (unsigned i = 0; i < num_watchers; i++) {
      if (!m_clause_write_unit[i]->out.empty()) {

        busy = true;
        auto &req = m_clause_write_unit[i]->out.front();
        assert(req->ComponentId >= num_watchers);
        auto source =
            (req->ComponentId - num_watchers) / (num_clauses / num_watchers);
        req->icnt_from = source;
        auto addr = get_addr_by_req(req);
        req->icnt_to =
            num_watchers + get_partition_id_by_addr(addr, num_partition);
        req->m_size = 12;
        total_memory_icnt_traffic += 64;
        memory_read_icnt->in_reqs[source].push_back(std::move(req));

        // m_cache_interface->in_request_queue.push_back(std::move(req));
        m_clause_write_unit[i]->out.pop_front();
      }
    }
    return busy;
  });
}

void acc::add_hook_from_watcher_write_unit_to_cache() {

#if __cplusplus >= 202002L
  // good
  clock_passes.push_back([=, this]() {
#else
  // from watceher write unit to l3 cache
  clock_passes.push_back([this]() {
#endif
    bool busy = false;
    for (unsigned i = 0; i < num_watchers; i++) {
      if (!m_watcher_write_unit[i]->out_mem_requst.empty()) {
        auto &req = m_watcher_write_unit[i]->out_mem_requst.front();
        auto source = i;
        req->icnt_from = source;
        assert(req);

        auto addr =
            req->as->get_is_push_to_other(req->watcherId)
                ? req->as->get_pushed_watcher_list_tail_addr(req->watcherId)
                : req->as->get_addr();
        req->icnt_to =
            num_watchers + get_partition_id_by_addr(addr, num_partition);
        req->m_size = 4;
        total_memory_icnt_traffic += 64;
        memory_read_icnt->in_reqs[source].push_back(std::move(req));

        m_watcher_write_unit[i]->out_mem_requst.pop_front();
        busy = true;
      }
    }
    return busy;
  });
}

void acc::add_hook_from_icnt_to_other() {
  clock_passes.push_back([this]() {
    auto busy = false;
    for (auto &&[i, watcher_unit] : enumerate(watchers)) {
      int remain_port = multi_l3cache_port;

      // this for loop: for each watcher unit
      while (!memory_read_icnt->out_reqs[i].empty() and
             watcher_unit->recieve_mem_rdy() and remain_port-- > 0) {

        // this if: this is from icnt to watchers or clauses or private cache
        busy = true;
        auto &req = memory_read_icnt->out_reqs[i].front(); // get the owner;
        if (req->ComponentId < num_watchers) {
          // bug here
          // need to send to private cache if it's value accesss
          if (req->type == AccessType::ReadWatcherValue) {
#ifdef SJQ_ICNT_DEBUG
            std::cout << "req out from icnt to private cache,it's read watcher "
                         "value: "
                      << i << std::endl;
            std::cout << *req << std::endl;
#endif
            m_private_caches[i]->in_resp.push_back(std::move(req));
          } else if (req->type == AccessType::ReadWatcherMetaData) {
            watcher_unit->in_memory_resp_queue.push_back(std::move(req));
          } else {
#ifdef SJQ_ICNT_DEBUG
            std::cout
                << "req out from icnt to watcher unit,it's read watcher data "
                << i << std::endl;
            std::cout << *req << std::endl;
#endif
            assert(req->type == AccessType::ReadWatcherData);
            watcher_unit->in_memory_resp_queue.push_back(std::move(req));
          }
        } else {
          // it's clause access
          if (req->type == AccessType::ReadClauseValue) {
#ifdef SJQ_ICNT_DEBUG
            std::cout
                << "req out from icnt to clause unit,it's read clause value "
                << i << std::endl;
            std::cout << *req << std::endl;
#endif
            m_private_caches[i]->in_resp.push_back(std::move(req));
          } else {
            assert(req->type == AccessType::ReadClauseData);
            assert(req->ComponentId >= num_watchers &&
                   req->ComponentId < num_watchers + num_clauses);
            auto clause_id = (req->ComponentId - num_watchers);
            assert(clause_id / (num_clauses / num_watchers) == i);
#ifdef SJQ_ICNT_DEBUG
            std::cout
                << "req out from icnt to clause unit,it's read clause data "
                << i << std::endl;
            std::cout << *req << std::endl;

#endif
            clauses[clause_id]->push_to_in_memory_resp_queue(std::move(req));
          }
        }
        memory_read_icnt->out_reqs[i].pop_front();
      }
    }
    for (auto i = 0u; i < num_partition; i++) {
      int remain_port = multi_l3cache_port;
      while (!memory_read_icnt->out_reqs[i + num_watchers].empty() and
             m_cache_interface->recieve_rdy(i) and remain_port-- > 0) {

        auto &req = memory_read_icnt->out_reqs[i + num_watchers].front();
        busy = true;
#ifdef SJQ_ICNT_DEBUG
        std::cout << "req out from icnt into cache interface " << i
                  << std::endl;
        std::cout << *req << std::endl;
#endif
        m_cache_interface->in_request_queues[i].push_back(std::move(req));

        memory_read_icnt->out_reqs[i + num_watchers].pop_front();
      }
    }
    return busy;
  });
}

// TODO, currently the clause will not be associated to watcher
void acc::add_hook_from_watcher_icnt_out() {
  clock_passes.push_back([this]() -> bool {
    bool busy = false;
    for (unsigned i = 0; i < num_watchers; i++) {

      if (!watcher_to_clause_icnt->out_reqs[i].empty()) {
        busy = true;
        auto &req = watcher_to_clause_icnt->out_reqs[i].front();
        static int next_clause = 0;
        next_clause = (next_clause + 1) % (num_clauses / num_watchers);
        req->ComponentId = i * (num_clauses / num_watchers) + next_clause;
        assert(req->as->get_clause_id(req->watcherId) % num_watchers == i);

        clauses[i + next_clause]->push_to_in_task_queue(std::move(req));
        watcher_to_clause_icnt->out_reqs[i].pop_front();
      }
    }
    return busy;
  });
}

void acc::add_hook_from_watcher_icnt_to_watcher_writer() {
  // TODO
  clock_passes.push_back([this]() -> bool {
    bool busy = false;
    for (unsigned i = 0; i < num_watchers; i++) {

      if (!watcher_to_writer_icnt->out_reqs[i].empty()) {
        busy = true;
        auto &req = watcher_to_writer_icnt->out_reqs[i].front();
        m_watcher_write_unit[i]->in_request.push_back(std::move(req));
        // clauses[i]->in_task_queue.push_back(std::move(req));
        watcher_to_writer_icnt->out_reqs[i].pop_front();
      }
    }
    return busy;
  });
}

void acc::parse_file() {
  // po::options_description config_file_options("all options");
  // std::map<std::string, int> int_map;
  // std::map<std::string, std::string> string_map;

  // config_file_options.add_options()("help", "print help infomation")(
  //     "n_watchers", po::value<int>()->default_value(16)->notifier([&](int v)
  //     {
  //       int_map["n_watchers"] = v;
  //     }),
  //     "the number of watchers")("n_clauses",
  //                               po::value<int>()->default_value(16)->notifier(
  //                                   [&](int v) { int_map["n_clauses"] = v;
  //                                   }),
  //                               "the number of clauses")(
  //     "mems", po::value<int>()->default_value(8)->notifier([&](int v) {
  //       int_map["mems"] = v;
  //     }),
  //     "the number of mem partition")(
  //     "icnt",
  //     po::value<std::string>()->default_value("ideal")->notifier(
  //         [&](std::string v) { string_map["icnt"] = v; }),
  //     "the type of icnt")("seq", po::bool_switch()->default_value(false),
  //                         "if enable seq mode")(
  //     "ideal_memory", po::bool_switch()->default_value(false),
  //     "if use ideal memory")("ideal_l3cache",
  //                            po::bool_switch()->default_value(16),
  //                            "if use ideal l3 cache")(
  //     "multi_port", po::value<int>()->default_value(1)->notifier([&](int v) {
  //       int_map["multi_port"] = v;
  //     }),
  //     "the number of ports for l3 cache to read in one cycle")(
  //     "dram_config",
  //     po::value<std::string>()
  //         ->default_value("HBM-config.conf")
  //         ->notifier([&](std::string v) { string_map["dram_config"] = v; }),
  //     "name of the dram_config")(
  //     "watcher_to_clause_icnt",
  //     po::value<std::string>()->default_value("mesh")->notifier(
  //         [&](std::string v) { string_map["watcher_to_clause_icnt"] = v; }),
  //     "the type of the icnt connect watcher and clause")(
  //     "watcher_to_writer_icnt",
  //     po::value<std::string>()->default_value("mesh")->notifier(
  //         [&](std::string v) { string_map["watcher_to_writer_icnt"] = v; }),
  //     "the type of the icnt connect the watcher to the writer")(
  //     "num_writer_entry",
  //     po::value<int>()->default_value(64)->notifier(
  //         [&](int v) { int_map["num_writer_entry"] = v; }),
  //     "the number of entrys in writer")(
  //     "num_writer_merge",
  //     po::value<int>()->default_value(8)->notifier(
  //         [&](int v) { int_map["num_writer_merge"] = v; }),
  //     "the number of requests can be merged by one entry")(
  //     "single_watcher", po::bool_switch()->default_value(false),
  //     "restrice single watcher!");

  // const std::map<std::string, std::set<std::string>> all_settings = {
  //     {"n_watchers", {}},
  //     {"n_clauses", {}},
  //     {"mems", {}},
  //     {"icnt", {"mesh", "ideal", "ring"}},
  //     {"seq", {"true", "false"}},
  //     {"ideal_memory", {"true", "false"}},
  //     {"ideal_l3cache", {"true", "false"}},
  //     {"multi_port", {}},
  //     {"dram_config",
  //      {"ALDRAM-config.cfg", "DDR4-config.cfg", "GDDR5-config.cfg",
  //       "LPDDR3-config.cfg", "PCM-config.cfg", "STTMRAM-config.cfg",
  //       "WideIO2-config.cfg", "DDR3-config.cfg", "DSARP-config.cfg",
  //       "HBM-config.cfg", "LPDDR4-config.cfg", "SALP-config.cfg",
  //       "TLDRAM-config.cfg", "WideIO-config.cfg"}},
  //     {"watcher_to_clause_icnt", {"mesh", "ideal", "ring"}},
  //     {"watcher_to_writer_icnt", {"mesh", "ideal", "ring"}},
  //     {"num_writer_entry", {}},
  //     {"num_writer_merge", {}},
  //     {"single_watcher", {"true", "false"}}};

  // po::variables_map vm;
  // po::store(po::parse_config_file("satacc_config.txt", config_file_options),
  //           vm);
  // if (vm.count("help")) {
  //   std::cout << config_file_options << std::endl;
  //   std::cout.flush();
  //   exit(-1);
  // }

  // const std::string input_file_name = "satacc_config.txt";
  // std::ifstream in(input_file_name);
  // std::string icnt_type;
  // std::map<std::string, std::string> acc_config;
  // std::string line;
  // // read the config
  // while (std::getline(in, line)) {
  //   if (line[0] == '#' or line[0] == ',' or line[0] == ' ') {
  //     // this line is comment
  //     continue;
  //   }
  //   std::istringstream in_line(line);
  //   std::string key;
  //   if (std::getline(in_line, key, '=')) {

  //     std::string value;
  //     if (std::getline(in_line, value)) {
  //       acc_config.insert({key, value});
  //     }
  //   }
  // }
  // for (auto &&config : acc_config) {
  //   std::cout << fmt::format("{},{}\n", config.first, config.second);
  // }
  // // fisrt, valide the count
  // if (!(acc_config.size() == all_settings.size())) {
  //   throw std::runtime_error("the number of settings are not equal");
  // }
  // for (auto &&config : acc_config) {
  //   if (!all_settings.count(config.first)) {
  //     throw std::runtime_error("no such setting!");
  //   }
  //   if (!all_settings.at(config.first).empty() and
  //       !all_settings.at(config.first).count(config.second)) {
  //     throw std::runtime_error("no such value!");
  //   }
  // }

  num_watchers = rust_config.n_watchers;

  num_clauses = rust_config.n_clauses;

  // parse the number of mem partition
  unsigned num_mem;

  num_mem = rust_config.mems;
  num_writer_entry = rust_config.num_writer_entry;
  num_writer_merge = rust_config.num_writer_merge;

  num_partition = num_mem;
  // parse the icnt type

  auto icnt = rust_config.icnt;
  if (icnt == sjqrusttools::IcntType::Mesh) {
    memory_read_icnt = new icnt_mesh(
        current_cycle, num_watchers + num_partition, 3, 1, 0, 64, 3);
  } else if (icnt == sjqrusttools::IcntType::Ring) {
    memory_read_icnt = new icnt_ring(
        current_cycle, num_watchers + num_partition, 3, 1, 0, 64, 3);
  } else if (icnt == sjqrusttools::IcntType::Ideal) {
    memory_read_icnt =
        new icnt_ideal(current_cycle, num_watchers + num_partition);
  } else {
    memory_read_icnt = new icnt_mesh(
        current_cycle, num_watchers + num_partition, 3, 1, 0, 64, 3);
  }
  enable_sequential = rust_config.seq;
  ideal_memory = rust_config.ideal_memory;
  ideal_l3cache = rust_config.ideal_l3cache;
  single_watcher = rust_config.single_watcher;
  auto number = rust_config.multi_port;
  if (number > 1) {
    multi_l3cache_port = number;
  } else {
    multi_l3cache_port = 1;
  }

  auto dram_config_file_type = rust_config.dram_config;

  if (dram_config_file_type == sjqrusttools::DramType::DDR4) {
    dram_config_file = "DDR4-config.cfg";
  } else if (dram_config_file_type == sjqrusttools::DramType::HBM) {
    dram_config_file = "HBM-config.cfg";
  }
  auto watcher_icnt_s = rust_config.watcher_to_clause_icnt;
  if (watcher_icnt_s == sjqrusttools::IcntType::Mesh) {
    watcher_to_clause_icnt =
        new icnt_mesh(current_cycle, num_watchers, 3, 1, 0, 64, 3);
  } else if (watcher_icnt_s == sjqrusttools::IcntType::Ring) {
    watcher_to_clause_icnt =
        new icnt_ring(current_cycle, num_watchers, 3, 1, 0, 64, 3);
  } else if (watcher_icnt_s == sjqrusttools::IcntType::Ideal) {
    watcher_to_clause_icnt = new icnt_ideal(current_cycle, num_watchers);
  } else {
    throw;
  }

  watcher_icnt_s = rust_config.watcher_to_writer_icnt;
  if (watcher_icnt_s == sjqrusttools::IcntType::Mesh) {
    watcher_to_writer_icnt =
        new icnt_mesh(current_cycle, num_watchers, 3, 1, 0, 64, 3);
  } else if (watcher_icnt_s == sjqrusttools::IcntType::Ring) {
    watcher_to_writer_icnt =
        new icnt_ring(current_cycle, num_watchers, 3, 1, 0, 64, 3);
  } else if (watcher_icnt_s == sjqrusttools::IcntType::Ideal) {
    watcher_to_writer_icnt = new icnt_ideal(current_cycle, num_watchers);
  } else {
    throw;
  }
}

acc::acc(uint64_t &tcurrent_cycle)
    : componet(tcurrent_cycle),
      rust_config(sjqrusttools::config_from_file("satacc_config.toml")),
      private_cache_size(rust_config.private_cache_size),
      num_watchers(rust_config.n_watchers), num_clauses(rust_config.n_clauses) {
  // show the config
  sjqrusttools::show_config(&rust_config);
  // read the configure file. will set num_watcher and num_clause again. so
  // the previouse value will be override!
  parse_file();
  // parse rust config
  sjq::inside_busy = false;

  // add the componets s

  m_cache_interface = new cache_interface(
      rust_config.l3_cache_size, num_partition, ideal_memory, ideal_l3cache,
      multi_l3cache_port, dram_config_file, current_cycle);
  m_componets.push_back((componet *)m_cache_interface);
  for (unsigned i = 0; i < num_watchers; i++) {
    auto watcher_writer = new watcher_list_write_unit(
        current_cycle, num_writer_merge, num_writer_entry, num_watchers);
    m_watcher_write_unit.push_back(watcher_writer);
    m_componets.push_back(watcher_writer);
  }

  for (unsigned i = 0; i < num_watchers; i++) {
    auto tclause_writer = new clause_writer(current_cycle);
    m_clause_write_unit.push_back(tclause_writer);
    m_componets.push_back(tclause_writer);
  }

  // m_icnt = new icnt(tcurrent_cycle, t_num_watchers, 8, t_num_clauses);
  m_componets.push_back(memory_read_icnt);
  m_componets.push_back(watcher_to_clause_icnt);
  m_componets.push_back(watcher_to_writer_icnt);

  init_watcher_and_clause();

  add_hook_from_watcher_out_actions();
  add_hook_from_clause_to_mem();
  add_hook_from_cache_to_clause_and_watchers();
  add_hook_from_private_cache();
  add_hook_from_trail_to_watcher();
  add_hook_from_clause_to_trail();
  add_hook_from_watcher_to_writeuite();
  add_hook_from_clause_to_writeuint();
  add_hook_from_clause_write_unit_to_cache();
  add_hook_from_watcher_write_unit_to_cache();

  // new

  add_hook_from_icnt_to_other();
  // add hook from watcher icnt to others
  add_hook_from_watcher_icnt_out();
  add_hook_from_watcher_icnt_to_watcher_writer();
}

acc::~acc() {
  for (auto &comp : m_componets) {
    delete comp;
  }
}

bool acc::do_cycle() {
  bool busy = false;
  // data transimission for all the components
  // clock_passes setup at acc()
  for (auto &f : clock_passes) {
    busy |= f();
  }
  // clock all the internal components
  for (auto &c : m_componets) {
    busy |= c->cycle();
  }
  return busy;
}
