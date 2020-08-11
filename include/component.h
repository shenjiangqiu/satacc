#ifndef COMPONET_H
#define COMPONET_H
#include <iostream>
class componet
{
public:
    static uint64_t current_cycle;
    //bool busy;
    virtual ~componet() {}
    virtual bool cycle() = 0;
};
#endif