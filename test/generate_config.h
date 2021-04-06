#include <fstream>

void generate_config() {
  std::ofstream of("satacc_config.txt");
  of << "icnt=ideal\n\
watcher_to_clause_icnt=mesh\n\
watcher_to_writer_icnt=mesh\n\
#icnt=ideal,mesh,ring\n\
mems=8\n\
seq=false\n\
n_watchers=16\n\
n_clauses=16\n\
ideal_memory=false\n\
ideal_l3cache=false\n\
multi_port=1\n\
dram_config=DDR4-config.cfg\n\
num_writer_merge=8\n\
num_writer_entry=64\n\
single_watcher=false\n";
  of.close();
}