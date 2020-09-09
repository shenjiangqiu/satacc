#include "acc.h"
#include "set_up.h"
#include "cache_interface.h"
#include <iostream>

int main()
{
    uint64_t current_cycle = 0;
    auto m_acc = acc(4, 4, current_cycle);
    m_acc.current_cycle = 0;

    auto as = generate_wrap();
    auto req = std::make_unique<cache_interface_req>(ReadType::ReadWatcher, 0, 0, 0, as);
    m_acc.in_m_trail.push_back(std::move(req));
    bool print = false;
    while (!m_acc.empty())
    {
        //std::cout << m_acc.get_internal_size() << std::endl;
        m_acc.cycle();
        //std::cout << m_acc.current_cycle << std::endl;

        if (print)
        {
            std::cout << m_acc.get_internal_size();
        }
        m_acc.current_cycle++;
    }
    std::cout << "m_acc current cycle " << m_acc.current_cycle << std::endl;

    std::cout << m_acc.get_line_trace() << std::endl;
}