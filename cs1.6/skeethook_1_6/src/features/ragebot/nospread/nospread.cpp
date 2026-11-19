#include <cstrike1_5/math/cone.hpp>
#include <cstrike1_5/math/shared_random.hpp>

#include <features/ragebot/nospread/nospread.hpp>

namespace nospread
{
    auto capture(unsigned int random_seed) -> void
    {
        seed = random_seed;
    }

    auto available() -> bool
    {
        return seed != 0;
    }

    auto solvable(const vec3_t& view_angles, float spread_radius) -> bool
    {
        if (!available())
            return false;

        if (spread_radius <= 0.0f)
            return true;

        return math::cone::compensable(view_angles, offset(spread_radius, 1));
    }

    auto offset(float spread_radius, int future) -> vec2_t
    {
        if (spread_radius <= 0.0f)
            return vec2_t();

        return math::shared_random::spread(seed, future, spread_radius);
    }

    auto compensate(const vec3_t& view_angles, float spread_radius) -> vec3_t
    {
        if (!available())
            return view_angles;

        return math::cone::compensate(view_angles, offset(spread_radius, 1));
    }
}