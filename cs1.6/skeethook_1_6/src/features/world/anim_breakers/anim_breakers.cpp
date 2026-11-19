#include <cstrike1_5/enums.hpp>
#include <cstrike1_5/structs/client_state.hpp>
#include <cstrike1_5/structs/entity_state.hpp>
#include <features/vars.hpp>

#include <cstrike1_5/interfaces/engine/engine.hpp>
#include <cstrike1_5/interfaces/studio/studio.hpp>
#include <features/world/anim_breakers/anim_breakers.hpp>

namespace anim_breakers
{
    auto initialize() -> bool
    {
        if (patch_ready)
            return true;

        patch_ready = studio::set_draw_player(on_draw_player);

        return patch_ready;
    }

    auto shutdown() -> void
    {
        studio::restore_draw_player();
        patch_ready = false;
    }

    auto __cdecl on_draw_player(int flags, entity_state_t* pplayer) -> int
    {
        const int air = vars::anim_breakers.toggles[moonwalk_in_air];
        const int ground = vars::anim_breakers.toggles[slow_walk_on_ground];
        if (!air && !ground)
            return studio::draw_player_original(flags, pplayer);

        if (!pplayer || pplayer->number != engine::local_player_index())
            return studio::draw_player_original(flags, pplayer);

        client_state_t* state = engine::client_state();
        if (!state)
            return studio::draw_player_original(flags, pplayer);

        const usercmd_t& command = state->cmd;
        const bool moving = command.forwardmove != 0.0f || command.sidemove != 0.0f;
        const bool ducking = (command.buttons & in_duck) != 0;
        if (!moving || ducking)
            return studio::draw_player_original(flags, pplayer);

        const int onground = pplayer->onground;

        if (air && onground == -1)
            pplayer->gaitsequence = gait_run;

        if (ground && onground >= 0)
            pplayer->gaitsequence = gait_walk;

        return studio::draw_player_original(flags, pplayer);
    }
}