#include <cmath>

#include <cstrike1_5/enums.hpp>
#include <cstrike1_5/structs/client_state.hpp>
#include <cstrike1_5/structs/playermove.hpp>
#include <core/hooks.hpp>
#include <features/vars.hpp>

#include <features/misc/texturebug.hpp>

auto texturebug::reset() -> void
{
    active_this_tick = false;
}

auto texturebug::on_create_move(usercmd_t* cmd) -> void
{
    active_this_tick = false;

    if (!cmd || !vars::misc.texturebug)
        return;

    const playermove_t* player_move = hooks::player_move();

    if (!player_move || player_move->movetype != movetype_walk || player_move->waterlevel >= 2 || player_move->onground != -1)
        return;

    active_this_tick = true;

    float upmove = vars::misc.texturebug_upmove;

    if (player_move->velocity.z > 0.0f)
        upmove += player_move->velocity.z;

    if (player_move->velocity.z < -2.0f)
        upmove += player_move->velocity.z + 2.0f;

    cmd->upmove = upmove;
}