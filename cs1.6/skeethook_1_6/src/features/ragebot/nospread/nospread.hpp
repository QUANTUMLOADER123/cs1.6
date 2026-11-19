#pragma once

#include <cstdint>

#include <cstrike1_5/math/vector.hpp>

namespace nospread
{
    inline unsigned int seed = 0;

    auto capture(unsigned int random_seed) -> void;
    auto available() -> bool;
    auto solvable(const vec3_t& view_angles, float spread_radius) -> bool;
    auto offset(float spread_radius, int future) -> vec2_t;
    auto compensate(const vec3_t& view_angles, float spread_radius) -> vec3_t;
}