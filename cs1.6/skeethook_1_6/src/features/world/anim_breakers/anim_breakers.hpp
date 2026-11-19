#pragma once

#include <cstrike1_5/structs/entity_state.hpp>

namespace anim_breakers
{
    inline const char* items[] =
    {
        "moonwalk in air",
        "slow walk on ground",
    };

    inline constexpr int moonwalk_in_air = 0;
    inline constexpr int slow_walk_on_ground = 1;

    inline bool patch_ready = false;

    auto initialize() -> bool;
    auto shutdown() -> void;
    auto __cdecl on_draw_player(int flags, entity_state_t* pplayer) -> int;
}