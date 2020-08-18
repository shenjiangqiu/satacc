#ifndef FMT_TYPES_H
#define FMT_TYPES_H
#include <fmt/format.h>
#include <cache.h>
#include <string>
template <>
struct fmt::formatter<statistics> : formatter<std::string>
{
    // parse is inherited from formatter<string_view>.
    template <typename FormatContext>
    auto format(statistics c, FormatContext &ctx)
    {
        std::string r = fmt::format("c.num_hit {} ,c.num_hit_reserved {}  ,c.num_miss {} ,c.num_res_fail{} \n", c.num_hit, c.num_hit_reserved, c.num_miss, c.num_res_fail);

        return formatter<string_view>::format(r, ctx);
    }
};

#endif