#pragma once

#include <cmath>
#include <algorithm>

#include <cstrike1_5/math/vector.hpp>

namespace math
{
    inline constexpr float pi = 3.14159265358979323846f;

    inline auto to_radians(float degrees) -> float
    {
        return degrees * (pi / 180.0f);
    }

    inline auto to_degrees(float radians) -> float
    {
        return radians * (180.0f / pi);
    }

    inline auto normalize_angle(float angle) -> float
    {
        while (angle > 180.0f)
            angle -= 360.0f;
        while (angle < -180.0f)
            angle += 360.0f;

        return angle;
    }

    inline auto vector_angles(const vec3_t& source, const vec3_t& target) -> vec3_t
    {
        const vec3_t delta = target - source;
        vec3_t angles;
        angles.x = to_degrees(std::atan2(-delta.z, std::sqrtf(delta.x * delta.x + delta.y * delta.y)));
        angles.y = to_degrees(std::atan2(delta.y, delta.x));
        angles.z = 0.0f;
        angles.x = std::clamp(angles.x, -89.0f, 89.0f);

        return angles;
    }

    inline auto angle_delta(const vec3_t& from, const vec3_t& to) -> float
    {
        const float pitch = normalize_angle(to.x - from.x);
        const float yaw = normalize_angle(to.y - from.y);

        return std::sqrtf(pitch * pitch + yaw * yaw);
    }

    inline auto angle_vectors(const vec3_t& angles, vec3_t& forward, vec3_t& right, vec3_t& up) -> void
    {
        const float pitch_radians = to_radians(angles.x);
        const float yaw_radians = to_radians(angles.y);
        const float roll_radians = to_radians(angles.z);

        const float sin_pitch = std::sinf(pitch_radians);
        const float cos_pitch = std::cosf(pitch_radians);
        const float sin_yaw = std::sinf(yaw_radians);
        const float cos_yaw = std::cosf(yaw_radians);
        const float sin_roll = std::sinf(roll_radians);
        const float cos_roll = std::cosf(roll_radians);

        forward = vec3_t(cos_pitch * cos_yaw, cos_pitch * sin_yaw, -sin_pitch);
        right = vec3_t(-sin_roll * sin_pitch * cos_yaw + cos_roll * sin_yaw, -sin_roll * sin_pitch * sin_yaw - cos_roll * cos_yaw, -sin_roll * cos_pitch);
        up = vec3_t(cos_roll * sin_pitch * cos_yaw + sin_roll * sin_yaw, cos_roll * sin_pitch * sin_yaw - sin_roll * cos_yaw, cos_roll * cos_pitch);
    }
}
