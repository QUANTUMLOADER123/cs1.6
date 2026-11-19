#include <cmath>
#include <cfloat>
#include <algorithm>
#include <cstrike1_5/math/vector.hpp>
#include <cstrike1_5/math/angle.hpp>
#include <cstrike1_5/math/cone.hpp>
#include <cstrike1_5/enums.hpp>
#include <cstrike1_5/structs/playermove.hpp>
#include <cstrike1_5/structs/client_state.hpp>
#include <cstrike1_5/structs/studio.hpp>
#include <cstrike1_5/interfaces/engine/engine.hpp>
#include <cstrike1_5/interfaces/globals/globals.hpp>
#include <cstrike1_5/interfaces/studio/studio.hpp>
#include <cstrike1_5/interfaces/trace/trace.hpp>
#include <cstrike1_5/interfaces/weapons/weapons.hpp>
#include <cstrike1_5/classes/c_base_player.hpp>
#include <cstrike1_5/classes/c_weapon.hpp>
#include <cstrike1_5/spread/spread.hpp>
#include <core/hooks.hpp>
#include <features/vars.hpp>
#include <features/world/removals/removals.hpp>
#include <features/antihit/antihit.hpp>
#include <features/misc/misc.hpp>

#include <features/ragebot/ragebot.hpp>
#include <features/ragebot/nospread/nospread.hpp>
#include <features/ragebot/penetrate.hpp>
#include <features/ragebot/lag_record.hpp>

namespace ragebot
{
    auto eye_position() -> vec3_t
    {
        return c_base_player(engine::entity(engine::local_player_index())).get_eye_position();
    }

    auto is_shootable_weapon(int id) -> bool
    {
        return id != weapon_knife && id != weapon_c4 && id != weapon_hegrenade && id != weapon_smokegrenade && id != weapon_flashbang;
    }

    auto acquire_target(const vec3_t& view_angles) -> aim_result_t
    {
        aim_result_t best;
        float best_delta = FLT_MAX;

        const int local_index = engine::local_player_index();
        const int max_clients = engine::max_clients();

        c_base_player local(engine::entity(local_index));
        const vec3_t eye = local.get_eye_position();
        if (!eye.is_valid())
            return best;

        const float spread_radius = spread::radius(weapons::local(), is_in_zoom());
        const float minimum_damage = vars::ragebot.minimum_damage;
        const bool wants_penetration = vars::ragebot.penetration;
        const bool wants_backtrack = lag_record::available_window() > 0.0f;
        const int weapon_id = weapons::local().is_valid() ? weapons::local().get_id() : 0;
        const penetrate::bullet_t bullet = penetrate::bullet(weapon_id);

        const auto consider = [&](const vec3_t& point, float radius, int target_index)
        {
            if (!point.is_valid() || radius <= 0.0f)
                return;

            const vec3_t angles = math::vector_angles(eye, point);
            const float delta = math::angle_delta(view_angles, angles);
            const bool visible = trace::visible(eye, point);

            float damage = 0.0f;

            if (visible)
            {
                damage = calculate_damage(eye, point, target_index);

                if (damage <= 0.0f)
                    return;
            }
            else
            {
                if (!wants_penetration)
                    return;

                damage = penetrate::simulate(eye, point, bullet, target_index, local_index);

                if (damage <= 0.0f)
                    return;
            }

            if (damage < minimum_damage)
                return;

            if (visible && calculate_hit_chance(eye, point, radius, spread_radius) < vars::ragebot.hit_chance)
                return;

            if (delta >= best_delta)
                return;

            best_delta = delta;
            best.found = true;
            best.angles = angles;
            best.target_index = target_index;
        };

        for (int index = 1; index <= max_clients; index++)
        {
            if (index == local_index)
                continue;

            c_base_player player(engine::entity(index));
            if (!player.base || !player.is_player() || !player.is_alive() || !player.is_enemy())
                continue;

            studiohdr_t* studio_header = studio::header(index);
            if (studio_header && studio::has_model(index))
            {
                const mstudiobbox_t* hitboxes = studio::hitbox_list(studio_header);
                if (hitboxes)
                {
                    for (int hitbox = 0; hitbox < studio_header->numhitboxes; hitbox++)
                    {
                        const mstudiobbox_t& hitbox_data = hitboxes[hitbox];
                        if (hitbox_data.group < 1 || hitbox_data.group > 8 || !vars::ragebot.hitbox[hitbox_data.group - 1])
                            continue;

                        const vec3_t point = studio::hitbox_center(index, hitbox_data);
                        const float radius = ((hitbox_data.bbmax - hitbox_data.bbmin) * 0.5f).length();

                        consider(point, radius, index);
                    }
                }
            }

            if (wants_backtrack)
            {
                const int count = lag_record::record_count(index);
                for (int slot = 0; slot < count; slot++)
                {
                    const lag_record::record_t& record = lag_record::get_record(index, slot);
                    if (record.simulation_time > lag_record::reference_time())
                        continue;

                    vec3_t point = record.origin;
                    point.z += 16.0f;

                    consider(point, 16.0f, index);
                }
            }
        }

        return best;
    }

    auto current_clip() -> int
    {
        c_weapon weapon = weapons::local();
        if (!weapon.is_valid())
            return 0;

        client_state_t* state = engine::client_state();
        if (!state)
            return 0;

        const int id = weapon.get_id();
        if (id <= 0 || id >= 64)
            return 0;

        const local_state_t& prediction = state->predicted_frames[state->parsecountmod & 63];
        if (prediction.weapondata[id].m_iId != id)
            return 0;

        return prediction.weapondata[id].m_iClip;
    }

    auto shot_confirmed() -> bool
    {
        client_state_t* state = engine::client_state();
        c_weapon weapon = weapons::local();
        const int id = weapon.is_valid() ? weapon.get_id() : 0;
        if (!state || id <= 0 || id >= 64)
            return false;

        const local_state_t& prediction = state->predicted_frames[state->parsecountmod & 63];
        const weapon_data_t& weapon_data = prediction.weapondata[id];
        const float time = static_cast<float>(state->time);

        return weapon_data.m_iClip < clip_before_shot || weapon_data.m_flNextPrimaryAttack > time;
    }

    auto calculate_damage(const vec3_t& eye, const vec3_t& point, int target_index) -> float
    {
        const penetrate::bullet_t bullet = penetrate::bullet(weapons::local().get_id());
        if (bullet.damage <= 0.0f)
            return 0.0f;

        const trace_result_t result = trace::shot(eye, point, engine::local_player_index());
        if (result.entity_index != target_index || result.fraction >= 1.0f)
            return 0.0f;

        const float travelled = (result.endpos - eye).length();
        const float damage = std::roundf(bullet.damage * std::powf(bullet.falloff, travelled * 0.002f));

        return penetrate::hitgroup_damage(damage, result.hitgroup);
    }

    auto solve_spread(const vec3_t& eye, const vec3_t& target, const vec3_t& punch, float spread_radius, int target_index, vec3_t& command) -> bool
{
    if (target_index <= 0 || !nospread::solvable(target, spread_radius))
        return false;

    const vec3_t firing = nospread::compensate(target, spread_radius);

    command.x = std::clamp(firing.x - punch.x, -89.0f, 89.0f);
    command.y = math::normalize_angle(firing.y - punch.y);

    const vec3_t direction = math::cone::direction(command + punch, nospread::offset(spread_radius, 1));
    const trace_result_t result = trace::shot(eye, eye + direction * 8192.0f, engine::local_player_index());

    return result.entity_index == target_index && result.hitgroup != 0;
}

auto calculate_hit_chance(const vec3_t& eye, const vec3_t& point, float radius, float spread_radius) -> float
{
    if (vars::ragebot.nospread && nospread::available())
        return 100.0f;

    return math::cone::hit_chance(eye, point, radius, spread_radius);
}

    auto can_shoot() -> bool
    {
        c_weapon weapon = weapons::local();
        if (!weapon.is_valid())
            return false;

        const int id = weapon.get_id();
        if (id <= 0 || id >= 64 || !is_shootable_weapon(id))
            return false;

        client_state_t* state = engine::client_state();
        if (!state)
            return false;

        const local_state_t& prediction = state->predicted_frames[state->parsecountmod & 63];
        const weapon_data_t& weapon_data = prediction.weapondata[id];
        if (weapon_data.m_iId != id)
            return false;

        if (weapon_data.m_iClip <= 0 || weapon_data.m_fInReload || weapon_data.m_fInSpecialReload)
            return false;

        const float time = static_cast<float>(state->time);
        return weapon_data.m_flNextPrimaryAttack <= time && prediction.client.m_flNextAttack <= time;
    }

    auto is_scoped_weapon(int id) -> bool
    {
        return id == weapon_scout || id == weapon_awp || id == weapon_g3sg1 || id == weapon_sg550;
    }

    auto is_in_zoom() -> bool
    {
        const int* scoped_fov = removals::scoped_fov;
        return scoped_fov && *scoped_fov < 90;
    }

    auto apply_autostop(usercmd_t* cmd) -> void
    {
        playermove_t* player_move = hooks::player_move();

        if (vars::ragebot.autostop[0])
        {
            cmd->buttons |= in_duck;

            if (duck_ticks < 8)
                duck_ticks++;
        }
        else
        {
            duck_ticks = 0;
        }

        if (!player_move || player_move->onground == -1)
            return;

        if (!vars::ragebot.autostop[1] && !vars::ragebot.autostop[2])
            return;

        const float speed = player_move->velocity.length_2d();
        if (speed < 1.0f)
            return;

        if (vars::ragebot.autostop[1])
        {
            cmd->forwardmove = 0.0f;
            cmd->sidemove = 0.0f;
            return;
        }

        const float view_yaw = math::to_radians(cmd->viewangles.y);
        const float forward = player_move->velocity.x * std::cosf(view_yaw) + player_move->velocity.y * std::sinf(view_yaw);
        const float side = player_move->velocity.x * std::sinf(view_yaw) - player_move->velocity.y * std::cosf(view_yaw);

        const float length = std::sqrtf(forward * forward + side * side);
        if (length <= 0.0f || player_move->maxspeed <= 0.0f)
            return;

        const float scale = player_move->maxspeed / length;

        cmd->forwardmove = -forward * scale;
        cmd->sidemove = -side * scale;
    }

    auto autostop_done() -> bool
    {
        if (vars::ragebot.autostop[0] && duck_ticks < 8)
            return false;

        if (!vars::ragebot.autostop[1] && !vars::ragebot.autostop[2])
            return true;

        playermove_t* player_move = hooks::player_move();
        if (!player_move || player_move->onground == -1)
            return true;

        return player_move->velocity.length_2d() < 25.0f;
    }

    auto release_engagement() -> void
    {
        holding = false;
        duck_ticks = 0;
        zoom_queued = false;
        zoom_ticks = 0;
        shot_pending = false;
    }

    auto stance_between_shots(usercmd_t* cmd) -> void
    {
        if (vars::ragebot.autostop[3])
        {
            release_engagement();
            return;
        }

        if (holding)
            apply_autostop(cmd);
    }

    auto apply_autoscope(usercmd_t* cmd) -> void
    {
        if (!vars::ragebot.autoscope)
        {
            zoom_queued = false;
            zoom_ticks = 0;
            return;
        }

        c_weapon weapon = weapons::local();
        const int id = weapon.is_valid() ? weapon.get_id() : 0;
        if (!is_scoped_weapon(id))
        {
            zoom_queued = false;
            zoom_ticks = 0;
            return;
        }

        if (is_in_zoom())
        {
            zoom_queued = false;
            zoom_ticks = 0;
            return;
        }

        if (zoom_ticks > 0)
        {
            zoom_ticks--;
            return;
        }

        cmd->buttons |= in_attack2;
        zoom_queued = true;
        zoom_ticks = 8;
    }

    auto on_create_move(float frametime, usercmd_t* cmd) -> void
    {
        lag_record::update();

        if (!vars::ragebot.enabled)
        {
            shoot_held = false;
            wait_for_ready = false;
            shot_wait_ticks = 0;
            release_engagement();
            return;
        }

        if (shoot_held)
        {
            stance_between_shots(cmd);

            cmd->buttons &= static_cast<unsigned short>(~in_attack);
            shoot_held = false;

            if (shot_confirmed())
                wait_for_ready = true;
            else
                shot_wait_ticks = 8;

            return;
        }

        if (wait_for_ready)
        {
            stance_between_shots(cmd);

            if (!can_shoot())
                return;

            wait_for_ready = false;
            return;
        }

        if (shot_wait_ticks > 0)
        {
            stance_between_shots(cmd);
            shot_wait_ticks--;
            return;
        }

        const aim_result_t result = acquire_target(antihit::real_angles);

        if (!result.found || !can_shoot())
        {
            release_engagement();
            return;
        }

        holding = true;

        apply_autostop(cmd);
        apply_autoscope(cmd);

        if (zoom_queued)
            return;

        if (!autostop_done())
            return;

        const vec3_t punch = c_base_player().get_aim_punch();

        vec3_t aiming;

        if (vars::ragebot.nospread)
        {
            const c_weapon local = weapons::local();

            if (!solve_spread(eye_position(), result.angles, punch, spread::radius(local, is_in_zoom()), result.target_index, aiming))
            {
                release_engagement();
                return;
            }
        }
        else
        {
            aiming.x = std::clamp(result.angles.x - punch.x, -89.0f, 89.0f);
            aiming.y = math::normalize_angle(result.angles.y - punch.y);
        }

        clip_before_shot = current_clip();
        shoot_held = true;

        const float base_yaw = cmd->viewangles.y;
        cmd->viewangles = aiming;
        misc::movement_fix(cmd, aiming.y - base_yaw);
        cmd->buttons |= in_attack;

        shot_angles = result.angles;
        shot_tick = globals::tick_count();
        shot_pending = true;
    }
}