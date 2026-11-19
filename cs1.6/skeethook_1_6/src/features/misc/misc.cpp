#include <cmath>
#include <cstrike1_5/math/vector.hpp>
#include <cstrike1_5/math/angle.hpp>
#include <cstrike1_5/enums.hpp>
#include <cstrike1_5/structs/playermove.hpp>
#include <cstrike1_5/structs/client_state.hpp>
#include <core/hooks.hpp>
#include <features/vars.hpp>

#include <features/misc/misc.hpp>

namespace misc
{
    auto bunny_hop(usercmd_t* cmd, const playermove_t* player_move) -> void
    {
        if (player_move->onground == -1)
            cmd->buttons &= static_cast<unsigned short>(~in_jump);
    }

    auto autostrafe(usercmd_t* cmd, const playermove_t* player_move, bool climb) -> void
    {
        const float forward = cmd->forwardmove;
        const float side = cmd->sidemove;
        if (forward == 0.0f && side == 0.0f)
            return;

        const float view_yaw = cmd->viewangles.y;
        const float wish_yaw = wish_yaw_from_keys(forward, side, view_yaw);
        const float maxspeed = player_move->maxspeed;
        const float speed = player_move->velocity.length_2d();

        float strafe_yaw;
        if (speed < air_max_wishspeed)
        {
            strafe_yaw = wish_yaw;
            autostrafe_direction = math::normalize_angle(wish_yaw - autostrafe_turn_yaw) >= 0.0f ? 1 : -1;
        }
        else
        {
            const float velocity_yaw = math::to_degrees(std::atan2(player_move->velocity.y, player_move->velocity.x));

            if (autostrafe_turn_yaw == 0.0f || std::fabsf(math::normalize_angle(wish_yaw - autostrafe_turn_yaw)) > 90.0f)
                autostrafe_direction = math::normalize_angle(wish_yaw - velocity_yaw) >= 0.0f ? 1 : -1;

            strafe_yaw = velocity_yaw + static_cast<float>(autostrafe_direction) * 90.0f;
        }

        autostrafe_turn_yaw = strafe_yaw;

        const float angle = math::to_radians(view_yaw - strafe_yaw);
        cmd->forwardmove = std::cosf(angle) * maxspeed;
        cmd->sidemove = std::sinf(angle) * maxspeed;

        if (climb && vars::misc.autostrafe_speed > 1 && !vars::misc.bunny_hop)
            cmd->buttons |= static_cast<unsigned short>(in_jump);
    }

    auto wish_yaw_from_keys(float forwardmove, float sidemove, float view_yaw) -> float
    {
        return view_yaw - math::to_degrees(std::atan2(sidemove, forwardmove));
    }

    auto quick_stop(usercmd_t* cmd, const playermove_t* player_move) -> void
    {
        if (player_move->onground == -1)
            return;

        if (cmd->forwardmove != 0.0f || cmd->sidemove != 0.0f)
            return;

        const vec3_t velocity = player_move->velocity;
        if (velocity.length_2d() < 0.01f)
            return;

        const float view_yaw = math::to_radians(cmd->viewangles.y);
        const float forward_component = velocity.x * std::cosf(view_yaw) + velocity.y * std::sinf(view_yaw);
        const float side_component = velocity.x * std::sinf(view_yaw) - velocity.y * std::cosf(view_yaw);

        cmd->forwardmove = -forward_component;
        cmd->sidemove = -side_component;
    }

    auto jumpbug(usercmd_t* cmd, const playermove_t* player_move) -> void
    {
        if (player_move->onground != -1 || player_move->velocity.z >= 0.0f)
            return;

        cmd->buttons |= in_duck;
    }

    auto ground_strafe(usercmd_t* cmd, const playermove_t* player_move) -> void
    {
        if (player_move->onground == -1)
        {
            if (player_move->maxspeed > 0.0f)
                autostrafe(cmd, player_move, false);

            return;
        }

        const float forward = cmd->forwardmove;
        const float side = cmd->sidemove;
        if (forward == 0.0f && side == 0.0f)
            return;

        const float speed = player_move->velocity.length_2d();
        if (speed < player_move->maxspeed * 0.9f)
            return;

        if (++ground_strafe_duck_ticks == 1 || ground_strafe_duck_ticks == 2)
            cmd->buttons |= in_duck;
        else
            cmd->buttons &= static_cast<unsigned short>(~in_duck);

        if (ground_strafe_duck_ticks >= 3)
            ground_strafe_duck_ticks = 0;

        const float velocity_yaw = math::to_degrees(std::atan2(player_move->velocity.y, player_move->velocity.x));
        const float wish_yaw = wish_yaw_from_keys(forward, side, cmd->viewangles.y);
        const float delta = math::normalize_angle(wish_yaw - velocity_yaw);
        if (std::fabsf(delta) < 1.0f)
            return;

        const float strafe_yaw = velocity_yaw + (delta >= 0.0f ? 90.0f : -90.0f);
        const float angle = math::to_radians(cmd->viewangles.y - strafe_yaw);

        cmd->forwardmove = std::cosf(angle) * player_move->maxspeed;
        cmd->sidemove = std::sinf(angle) * player_move->maxspeed;
    }

    auto movement_fix(usercmd_t* cmd, float yaw_delta) -> void
    {
        if (yaw_delta == 0.0f)
            return;

        const float radians = math::to_radians(yaw_delta);
        const float cosine = std::cosf(radians);
        const float sine = std::sinf(radians);
        const float forward = cmd->forwardmove;
        const float side = cmd->sidemove;

        cmd->forwardmove = forward * cosine - side * sine;
        cmd->sidemove = forward * sine + side * cosine;
    }

    auto on_create_move(float frametime, usercmd_t* cmd) -> void
    {
        playermove_t* player_move = hooks::player_move();
        if (!player_move || player_move->movetype != movetype_walk || player_move->waterlevel >= 2)
            return;

        if (player_move->onground == -1)
        {
            autostrafe_direction = 0;
            autostrafe_turn_yaw = 0.0f;
        }

        if (vars::misc.jumpbug)
            jumpbug(cmd, player_move);

        if (vars::misc.bunny_hop)
            bunny_hop(cmd, player_move);

        if (vars::misc.ground_strafe)
            ground_strafe(cmd, player_move);

        if (vars::misc.quick_stop)
            quick_stop(cmd, player_move);

        if (vars::misc.autostrafe && player_move->onground == -1 && player_move->maxspeed > 0.0f)
            autostrafe(cmd, player_move, true);
    }

    auto initialize() -> bool
    {
        static bool initialized = false;
        if (initialized)
            return true;

        initialized = true;

        return true;
    }
}
