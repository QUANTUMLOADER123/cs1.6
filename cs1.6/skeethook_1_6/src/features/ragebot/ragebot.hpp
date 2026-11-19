#pragma once

#include <cstrike1_5/structs/client_state.hpp>
#include <cstrike1_5/math/vector.hpp>

namespace ragebot
{
    struct aim_result_t
    {
        bool found = false;
        vec3_t angles;
        int target_index = 0;
    };

    inline bool shoot_held = false;
    inline bool wait_for_ready = false;
    inline int clip_before_shot = 0;
    inline int shot_wait_ticks = 0;
    inline bool zoom_queued = false;
    inline int zoom_ticks = 0;
    inline vec3_t shot_angles{};
    inline int shot_tick = 0;
    inline bool shot_pending = false;
    inline bool holding = false;
    inline int duck_ticks = 0;

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

    inline const char* autostop_items[] =
    {
        "duck",
        "full",
        "strafe",
        "move between shots",
    };

    auto on_create_move(float frametime, usercmd_t* cmd) -> void;
    auto eye_position() -> vec3_t;
    auto acquire_target(const vec3_t& view_angles) -> aim_result_t;
    auto current_clip() -> int;
    auto shot_confirmed() -> bool;
    auto can_shoot() -> bool;
    auto calculate_hit_chance(const vec3_t& eye, const vec3_t& point, float radius, float spread_radius) -> float;
    auto calculate_damage(const vec3_t& eye, const vec3_t& point, int target_index) -> float;
    auto solve_spread(const vec3_t& eye, const vec3_t& target, const vec3_t& punch, float spread_radius, int target_index, vec3_t& command) -> bool;
    auto is_shootable_weapon(int id) -> bool;
    auto is_scoped_weapon(int id) -> bool;
    auto is_in_zoom() -> bool;
    auto apply_autostop(usercmd_t* cmd) -> void;
    auto autostop_done() -> bool;
    auto release_engagement() -> void;
    auto stance_between_shots(usercmd_t* cmd) -> void;
    auto apply_autoscope(usercmd_t* cmd) -> void;
}