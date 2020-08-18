#ifndef COMPONET_H
#define COMPONET_H
#include <iostream>
#include <string>
class componet
{
public:
    virtual std::string get_internal_size() const = 0;
    virtual double get_busy_percent() const = 0;
    virtual std::string get_line_trace() const = 0;

    componet(uint64_t &tcurrent_cycle) : current_cycle(tcurrent_cycle) {}
    uint64_t &current_cycle;
    //static uint64_t current_cycle;
    //bool busy;
    virtual bool empty() const = 0;
    virtual ~componet() {}
    virtual bool cycle() = 0;
};
#endif