#pragma once

#include <cstrike1_5/structs/client_state.hpp>
#include <cstrike1_5/structs/playermove.hpp>

namespace misc
{
    inline int ground_strafe_duck_ticks = 0;
    inline int autostrafe_direction = 0;
    inline float autostrafe_turn_yaw = 0.0f;
    inline const float air_max_wishspeed = 30.0f;

    auto bunny_hop(usercmd_t* cmd, const playermove_t* player_move) -> void;
    auto autostrafe(usercmd_t* cmd, const playermove_t* player_move, bool climb) -> void;
    auto quick_stop(usercmd_t* cmd, const playermove_t* player_move) -> void;
    auto jumpbug(usercmd_t* cmd, const playermove_t* player_move) -> void;
    auto wish_yaw_from_keys(float forwardmove, float sidemove, float view_yaw) -> float;
    auto ground_strafe(usercmd_t* cmd, const playermove_t* player_move) -> void;
    auto movement_fix(usercmd_t* cmd, float yaw_delta) -> void;
    auto on_create_move(float frametime, usercmd_t* cmd) -> void;

    auto initialize() -> bool;
}
