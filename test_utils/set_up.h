#ifndef SET_UP_H
#define SET_UP_H

#include "assign_wrap.h"
#include <tuple>
assign_wrap *generate_wrap();
assign_wrap *generate_wrap_short();
std::pair<assign_wrap *, assign_wrap *> generate_wrap_para();
#endif