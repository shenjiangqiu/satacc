#include "read_config.h"
#include <fstream>
#include <sstream>
namespace sjq
{
    std::istringstream &operator>>(std::istringstream &is, config &m_config)
    {
        is >> m_config.watcher_proc_size;
        is >> m_config.watcher_proc_num;
        is >> m_config.clause_proc_num;
        is >> m_config.miss_latency;
        is >> m_config.watcher_process_latency;
        is >> m_config.clause_process_latency;
        is >> m_config.vault_memory_access_latency;
        is >> m_config.cpu_to_vault_latency;
        is >> m_config.mode2;
        is >> m_config.ctr_latency;
        is >> m_config.use_single_watcher;
        return is;
    }

    std::vector<sjq::config> parse_file(const std::string &filename)
    {
        auto file_stream = std::ifstream(filename);
        if (!file_stream)
        {
            throw std::runtime_error("no config file find");
            return std::vector<sjq::config>();
        }
        auto result = std::vector<sjq::config>();
        config m_config;
        std::string line;
        while (std::getline(file_stream, line))
        {
            if (line[0] == '#')
                continue;
            if (line[0] == ' ')
                continue;
            if (line.size() < 5)
                continue;
            std::istringstream i_line(line);
            i_line >> m_config;
            result.push_back(m_config);
        }
        return result;
    }

} // namespace sjq