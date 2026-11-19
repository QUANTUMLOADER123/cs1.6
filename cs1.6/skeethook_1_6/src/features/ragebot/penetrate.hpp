#pragma once

#include <cstrike1_5/math/vector.hpp>

namespace penetrate
{
    struct bullet_t
    {
        float damage;
        float range;
        float falloff;
        float max_distance;
        float penetration;
        int max_penetration;
    };

    auto bullet(int weapon_id) -> bullet_t;
    auto hitgroup_damage(float damage, int hitgroup) -> float;
    auto simulate(const vec3_t& start, const vec3_t& end, const bullet_t& bullet, int target_index, int shooter_index) -> float;
}
