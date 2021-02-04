#ifndef READCONFIG_H
#define READCONFIG_H
#include <iostream>
#include <vector>

namespace sjq{
    struct config{
       int watcher_proc_size;
       int watcher_proc_num;
       int clause_proc_num;
       int miss_latency;
       int watcher_process_latency;
       int clause_process_latency;
       int vault_memory_access_latency;
       int cpu_to_vault_latency;
       bool mode2;
       int ctr_latency;
       bool use_single_watcher;
    };
    std::vector<config> parse_file(const std::string &filename);
}
#endif

