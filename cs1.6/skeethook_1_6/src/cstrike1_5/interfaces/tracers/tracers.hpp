#pragma once

#include <cstrike1_5/math/vector.hpp>

namespace engine_tracers
{
    using user_tracer_t = void(__cdecl*)(const vec3_t*, const vec3_t*, float, int, float, char, int);

    inline user_tracer_t user_tracer = nullptr;

    auto initialize() -> bool;
    auto add(const vec3_t& start, const vec3_t& direction, float life, int color, float speed, char scale, int alpha) -> bool;
}
