#include "cache_interface.h"
#include "component.h"
#include <functional>
#include <icnt_wrapper.h>
#include <enumerate.h>

cache_interface::cache_interface(unsigned int total_size,unsigned num_partition,
                                 uint64_t &t) : cache_interface(16, total_size >> 10, 192, 4,num_partition, t)
{
}
