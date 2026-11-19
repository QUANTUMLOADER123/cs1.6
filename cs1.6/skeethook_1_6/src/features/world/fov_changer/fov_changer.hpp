#pragma once

namespace fov_changer
{
    inline float* fov_target = nullptr;
    inline float* fov_current = nullptr;
    inline float* fov_previous = nullptr;
    inline float* fov_scaled = nullptr;
    inline float last_custom_fov = 90.0f;

    auto initialize() -> bool;
    auto poll() -> void;
}