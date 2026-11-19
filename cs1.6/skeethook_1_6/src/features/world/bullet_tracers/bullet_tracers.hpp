#pragma once

#include <vector>

#include <core/hooks.hpp>
#include <cstrike1_5/math/vector.hpp>

namespace bullet_tracers
{
    struct impact_t
    {
        vec3_t point;
        double created;
    };

    inline std::vector<impact_t> impacts = {};

    inline hook fire_bullets_hook;
    inline hook bullet_impact_hook;
    inline bool own_fire_active = false;

    auto initialize() -> bool;
    auto shutdown() -> void;
    auto render() -> void;
    auto angle_basis(const vec3_t& angles, vec3_t& forward, vec3_t& right, vec3_t& up) -> void;
}