#pragma once

#include <cstrike1_5/structs/client_state.hpp>

namespace texturebug
{
    inline bool active_this_tick = false;

    auto reset() -> void;
    auto on_create_move(usercmd_t* cmd) -> void;
}
