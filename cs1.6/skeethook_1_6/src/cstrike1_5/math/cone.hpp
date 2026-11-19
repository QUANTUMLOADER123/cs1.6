#pragma once

#include <cmath>
#include <algorithm>

#include <cstrike1_5/math/angle.hpp>
#include <cstrike1_5/math/vector.hpp>

namespace math
{
    namespace cone
    {
        inline auto clamp_cosine(float value) -> float
        {
            return std::clamp(value, -1.0f, 1.0f);
        }

        inline auto radius_at(float distance, float spread) -> float
        {
            return distance * spread;
        }

        inline auto hit_chance(float cone_radius, float target_radius, float miss_distance) -> float
        {
            if (cone_radius <= 0.0f)
                return 100.0f;

            if (miss_distance + cone_radius <= target_radius)
                return 100.0f;

            if (miss_distance + target_radius <= cone_radius)
                return std::clamp(target_radius * target_radius * 100.0f / (cone_radius * cone_radius), 0.0f, 100.0f);

            if (miss_distance >= cone_radius + target_radius)
                return 0.0f;

            const float squared_miss = miss_distance * miss_distance;
            const float squared_cone = cone_radius * cone_radius;
            const float squared_target = target_radius * target_radius;

            const float cone_term = squared_cone * std::acos(clamp_cosine((squared_miss + squared_cone - squared_target) / (2.0f * miss_distance * cone_radius)));
            const float target_term = squared_target * std::acos(clamp_cosine((squared_miss + squared_target - squared_cone) / (2.0f * miss_distance * target_radius)));

            const float lens = (-miss_distance + cone_radius + target_radius) * (miss_distance + cone_radius - target_radius) * (miss_distance - cone_radius + target_radius) * (miss_distance + cone_radius + target_radius);
            const float triangle_term = 0.5f * std::sqrtf(std::max(0.0f, lens));

            return std::clamp((cone_term + target_term - triangle_term) * 100.0f / (pi * squared_cone), 0.0f, 100.0f);
        }

        inline auto hit_chance(const vec3_t& eye, const vec3_t& point, float target_radius, float spread) -> float
        {
            return hit_chance(radius_at((point - eye).length(), spread), target_radius, 0.0f);
        }

        inline auto direction(const vec3_t& view_angles, const vec2_t& offset) -> vec3_t
        {
            vec3_t forward, right, up;
            angle_vectors(view_angles, forward, right, up);

            return (forward + right * offset.x + up * offset.y).normalized();
        }

        inline auto compensable(const vec3_t& view_angles, const vec2_t& offset) -> bool
        {
            const float length = std::sqrt(1.0f + offset.x * offset.x + offset.y * offset.y);
            const float vertical = std::sqrt(1.0f + offset.y * offset.y);
            const float ratio = length * -std::sinf(to_radians(view_angles.x)) / vertical;

            return ratio >= -1.0f && ratio <= 1.0f;
        }

        inline auto compensate(const vec3_t& view_angles, const vec2_t& offset) -> vec3_t
        {
            if (!compensable(view_angles, offset))
                return view_angles;

            const float length = std::sqrt(1.0f + offset.x * offset.x + offset.y * offset.y);
            const float vertical = std::sqrt(1.0f + offset.y * offset.y);
            const float ratio = clamp_cosine(length * -std::sinf(to_radians(view_angles.x)) / vertical);

            const float base = std::atan2(offset.y, 1.0f);
            const float branch = std::asin(ratio);
            const float direct = base - branch;
            const float flipped = base - (pi - branch);
            const float solved_pitch = to_degrees(std::fabs(direct) <= std::fabs(flipped) ? direct : flipped);

            const float solved_radians = to_radians(solved_pitch);
            const float projection = std::cosf(solved_radians) + offset.y * std::sinf(solved_radians);
            const float solved_yaw = normalize_angle(view_angles.y + to_degrees(std::atan2(offset.x, projection)));

            return vec3_t(std::clamp(solved_pitch, -89.0f, 89.0f), solved_yaw, 0.0f);
        }
    }
}
