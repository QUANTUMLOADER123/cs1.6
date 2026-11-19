#pragma once

#include <cstrike1_5/math/vector.hpp>

namespace renderer
{
    static const float* world_matrix = nullptr;
    static const int* screen_origin_x = nullptr;
    static const int* screen_origin_y = nullptr;
    static const int* screen_width = nullptr;
    static const int* screen_height = nullptr;
    static const int* viewport_x = nullptr;
    static const int* viewport_y = nullptr;


    auto initialize() -> bool;
    auto world_to_screen(const vec3_t& world, vec2_t& screen) -> bool;
}
