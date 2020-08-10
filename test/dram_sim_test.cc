#include "memory_system.h"
#include <catch.hpp>
#include <iostream>
bool recieved = false;
int waiting = 4;
int i = 0;

void read_call_back(uint64_t addr)
{
    std::cout << "recieveed " << addr << ", clock:" << i << std::endl;
    waiting--;
    recieved = true;
}
void write_call_back(uint64_t addr)
{
    std::cout << "recieveed " << addr << std::endl;
    recieved = true;
}
TEST_CASE("DramTest")
{

    dramsim3::MemorySystem m_mem("DDR4_4Gb_x16_2133_2.ini", "./", std::bind(read_call_back, std::placeholders::_1), std::bind(write_call_back, std::placeholders::_1));
    m_mem.AddTransaction(123, false);
    while (!m_mem.WillAcceptTransaction(333, false))
    {
        m_mem.ClockTick();
        i++;
        /* code */
    }
    m_mem.AddTransaction(333, false);
    while (!m_mem.WillAcceptTransaction(333, false))
    {
        m_mem.ClockTick();
        i++;
        /* code */
    }
    m_mem.AddTransaction(124, false);
    while (!m_mem.WillAcceptTransaction(333, false))
    {
        m_mem.ClockTick();
        i++;
        /* code */
    }
    m_mem.AddTransaction(125, false);
    while (waiting > 0)
    {
        i++;
        m_mem.ClockTick();
    }
    std::cout << "clock: " << i << std::endl;
    m_mem.PrintStats();
    std::cout << m_mem.GetTCK() << std::endl;
}