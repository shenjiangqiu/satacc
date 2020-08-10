#ifndef COMPONET_H
#define COMPONET_H

class componet
{
public:
    static unsigned long long current_cycle;
    bool busy;

    virtual void cycle() = 0;


};
#endif