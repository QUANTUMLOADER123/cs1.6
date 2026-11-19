#include <cstdint>
#include <cstrike1_5/math/vector.hpp>
#include <cstrike1_5/modules.hpp>
#include <cstrike1_5/common.hpp>
#include <memory/scanner.hpp>

#include <cstrike1_5/interfaces/tracers/tracers.hpp>

auto engine_tracers::initialize() -> bool
{
    if (user_tracer)
        return true;

    uint8_t* match = memory::find_pattern(modules::renderer, patterns::user_tracer);
    if (!match)
        return false;

    user_tracer = reinterpret_cast<user_tracer_t>(match);

    return true;
}

auto engine_tracers::add(const vec3_t& start, const vec3_t& direction, float life, int color, float speed, char scale, int alpha) -> bool
{
    if (!user_tracer)
        return false;

    user_tracer(&start, &direction, life, color, speed, scale, alpha);

    return true;
}
