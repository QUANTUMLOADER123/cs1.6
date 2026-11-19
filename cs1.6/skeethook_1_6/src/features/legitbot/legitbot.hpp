#pragma once

#include <cstrike1_5/structs/client_state.hpp>
#include <cstrike1_5/math/vector.hpp>

namespace legitbot
{
    struct aim_result_t
    {
        bool found = false;
        float delta = 0.0f;
        vec3_t angles;
    };

    inline const char* activation_items[] =
    {
        "always",
        "on attack",
    };

    inline const char* hitbox_items[] =
    {
        "head",
        "chest",
        "stomach",
        "left arm",
        "right arm",
        "left leg",
        "right leg",
        "weapon",
    };

    auto on_create_move(float frametime, usercmd_t* cmd) -> void;
    auto eye_position() -> vec3_t;
    auto acquire_target(const vec3_t& view_angles) -> aim_result_t;
    auto apply_smooth(const vec3_t& current, const vec3_t& target, vec3_t& out) -> void;
    auto render() -> void;
}