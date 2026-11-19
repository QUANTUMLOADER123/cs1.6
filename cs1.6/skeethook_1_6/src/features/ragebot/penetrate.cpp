#include <cmath>
#include <cstrike1_5/math/vector.hpp>
#include <cstrike1_5/enums.hpp>
#include <cstrike1_5/structs/studio.hpp>
#include <cstrike1_5/interfaces/trace/trace.hpp>
#include <features/ragebot/penetrate.hpp>

namespace penetrate
{
    auto bullet(int weapon_id) -> bullet_t
    {
        switch (weapon_id)
        {
        case weapon_ak47:
            return { 39.0f, 8192.0f, 0.98f, 5000.0f, 0.45f, 2 };
        case weapon_awp:
            return { 45.0f, 8192.0f, 0.99f, 8000.0f, 0.55f, 3 };
        case weapon_g3sg1:
            return { 39.0f, 8192.0f, 0.98f, 5000.0f, 0.50f, 3 };
        case weapon_m4a1:
            return { 35.0f, 8192.0f, 0.97f, 4000.0f, 0.45f, 2 };
        case weapon_aug:
            return { 35.0f, 8192.0f, 0.96f, 4000.0f, 0.45f, 2 };
        case weapon_sg552:
            return { 35.0f, 8192.0f, 0.955f, 4000.0f, 0.45f, 2 };
        case weapon_sg550:
            return { 35.0f, 8192.0f, 0.98f, 4000.0f, 0.50f, 2 };
        case weapon_famas:
            return { 35.0f, 8192.0f, 0.96f, 4000.0f, 0.45f, 2 };
        case weapon_galil:
            return { 35.0f, 8192.0f, 0.98f, 4000.0f, 0.45f, 2 };
        case weapon_m249:
            return { 30.0f, 8192.0f, 0.97f, 3000.0f, 0.40f, 2 };
        case weapon_scout:
            return { 39.0f, 8192.0f, 0.98f, 5000.0f, 0.50f, 3 };
        case weapon_deagle:
            return { 30.0f, 4096.0f, 0.81f, 1500.0f, 0.30f, 2 };
        case weapon_p228:
            return { 30.0f, 4096.0f, 0.80f, 1000.0f, 0.25f, 1 };
        case weapon_fiveseven:
            return { 30.0f, 4096.0f, 0.885f, 1000.0f, 0.25f, 1 };
        case weapon_usp:
            return { 30.0f, 4096.0f, 0.79f, 900.0f, 0.25f, 1 };
        case weapon_glock:
        case weapon_glock18:
            return { 30.0f, 8192.0f, 0.75f, 800.0f, 0.25f, 1 };
        case weapon_elite:
            return { 30.0f, 8192.0f, 0.75f, 800.0f, 0.25f, 1 };
        case weapon_p90:
            return { 30.0f, 8192.0f, 0.885f, 1500.0f, 0.30f, 1 };
        case weapon_mac10:
            return { 30.0f, 8192.0f, 0.82f, 1200.0f, 0.25f, 1 };
        case weapon_mp5navy:
            return { 30.0f, 8192.0f, 0.84f, 1200.0f, 0.25f, 1 };
        case weapon_tmp:
            return { 30.0f, 8192.0f, 0.85f, 1200.0f, 0.25f, 1 };
        case weapon_ump45:
            return { 30.0f, 8192.0f, 0.82f, 1200.0f, 0.25f, 1 };
        default:
            return { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0 };
        }
    }

    auto hitgroup_damage(float damage, int hitgroup) -> float
    {
        switch (hitgroup)
        {
        case hitgroup_head:
            return std::roundf(damage * 4.0f);
        case hitgroup_chest:
            return std::roundf(damage);
        case hitgroup_stomach:
            return std::roundf(damage * 1.25f);
        case hitgroup_left_arm:
        case hitgroup_right_arm:
        case hitgroup_left_leg:
        case hitgroup_right_leg:
        case hitgroup_weapon:
            return std::roundf(damage * 0.75f);
        default:
            return 0.0f;
        }
    }

    auto simulate(const vec3_t& start, const vec3_t& end, const bullet_t& bullet, int target_index, int shooter_index) -> float
    {
        if (bullet.damage <= 0.0f || bullet.max_penetration <= 0)
            return 0.0f;

        const vec3_t span = end - start;
        const float length = span.length();
        if (length <= 0.0f)
            return 0.0f;

        const vec3_t direction = span / length;

        vec3_t position = start;
        float damage = bullet.damage;

        for (int layer = 0; layer <= bullet.max_penetration; layer++)
        {
            const trace_result_t result = trace::shot(position, end, shooter_index);
            const float travelled = (result.endpos - position).length();

            if (travelled > 0.0f)
            {
                damage = std::roundf(damage * std::powf(bullet.falloff, travelled * 0.002f));

                if (damage <= 0.0f)
                    return 0.0f;
            }

            if (result.entity_index == target_index)
                return hitgroup_damage(damage, result.hitgroup);

            if (result.entity_index > 0)
                return 0.0f;

            if (result.fraction >= 1.0f)
                return 0.0f;

            const vec3_t entry = result.endpos + direction * 0.5f;
            const float thickness = trace::solid_travel(entry, direction, bullet.max_distance);

            if (thickness <= 0.0f)
                return 0.0f;

            damage = std::roundf(damage * std::powf(bullet.penetration, thickness * 0.01f));

            if (damage <= 0.0f)
                return 0.0f;

            position = entry + direction * (thickness + 0.5f);
        }

        return 0.0f;
    }
}
