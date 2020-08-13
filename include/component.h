#ifndef COMPONET_H
#define COMPONET_H
#include <iostream>
class componet
{
public:
    componet(uint64_t &tcurrent_cycle) : current_cycle(tcurrent_cycle) {}
    uint64_t &current_cycle;
    //static uint64_t current_cycle;
    //bool busy;
    virtual ~componet() {}
    virtual bool cycle() = 0;
};
#endif