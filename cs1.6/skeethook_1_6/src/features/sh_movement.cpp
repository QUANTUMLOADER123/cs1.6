#include "../../include/sh_movement.h"
#include "../../include/sh_math.h"
#include "../../include/sh_config.h"
#include "../../include/sh_ballistics.h"

#define sh_air_accel_cap 30.0f
#define sh_max_move 450.0f
#define sh_pi 3.14159265358979f

static f32 sh_atan2_deg(f32 y, f32 x)
{
    if (x == 0.0f)
        return (y > 0.0f) ? 90.0f : ((y < 0.0f) ? -90.0f : 0.0f);

    f32 a = sh_atan(y / x) * (180.0f / sh_pi);

    if (x < 0.0f)
        a += (y >= 0.0f) ? 180.0f : -180.0f;

    return a;
}

static f32 sh_cos_deg(f32 d) { return sh_cos(d * (sh_pi / 180.0f)); }
static f32 sh_sin_deg(f32 d) { return sh_sin(d * (sh_pi / 180.0f)); }

f32 sh_strafe_omega(f32 wishspeed)
{
    if (wishspeed <= 0.0f)
        return 0.0f;

    return 90.0f - 180.0f / (wishspeed / sh_air_accel_cap + 1.0f);
}

f32 sh_friction_drop(f32 speed, f32 frametime)
{
    f32 friction = 4.0f;
    f32 stopspeed = 75.0f;
    f32 control = (speed < stopspeed) ? stopspeed : speed;
    return friction * (control * frametime);
}

void sh_autostrafe(sh_usercmd_t *cmd, sh_playermove_t *pm)
{
    if (!g_cfg.movement.autostrafe || !cmd || !pm)
        return;

    if (pm->onground != -1)
        return;

    f32 vx = pm->velocity.x;
    f32 vy = pm->velocity.y;
    f32 speed = sh_sqrt(vx * vx + vy * vy);

    if (speed < 1.0f)
        return;

    f32 fm = cmd->forwardmove;
    f32 sm = cmd->sidemove;

    if (fm == 0.0f && sm == 0.0f)
        return;

    f32 wish_speed = sh_sqrt(fm * fm + sm * sm);
    f32 wish_yaw = sh_atan2_deg(sm, fm);
    f32 wish_dir = cmd->viewangles.y + wish_yaw;
    f32 vel_yaw = sh_atan2_deg(vy, vx);

    f32 omega = sh_strafe_omega(wish_speed);
    f32 d = sh_angle_delta(wish_dir, vel_yaw);

    if (d > 0.0f)
        omega = -omega;

    f32 target = vel_yaw + omega;

    cmd->forwardmove = sh_cos_deg(target) * sh_max_move;
    cmd->sidemove = sh_sin_deg(target) * sh_max_move;
}

void sh_quick_stop(sh_usercmd_t *cmd, sh_playermove_t *pm)
{
    if (!g_cfg.movement.quick_stop || !cmd || !pm)
        return;

    if (pm->onground == -1)
        return;

    f32 vx = pm->velocity.x;
    f32 vy = pm->velocity.y;
    f32 speed = sh_sqrt(vx * vx + vy * vy);

    if (speed < 1.0f)
        return;

    if (speed < pm->maxspeed * 0.9f)
        return;

    cmd->viewangles.y = sh_atan2_deg(vy, vx) + 180.0f;
    cmd->forwardmove = sh_max_move;
    cmd->sidemove = 0.0f;
}

void sh_bhop(sh_usercmd_t *cmd, sh_playermove_t *pm)
{
    if (!g_cfg.movement.bhop || !cmd || !pm)
        return;

    if (pm->onground == -1)
        return;

    cmd->buttons |= 2;
}

void sh_auto_jump_hold(sh_usercmd_t *cmd, sh_playermove_t *pm)
{
    if (!g_cfg.movement.auto_jump || !cmd || !pm)
        return;

    if (pm->onground == -1)
        cmd->buttons |= 2;
}

i32 g_shots_fired = 0;
static f32 g_last_attack_time = 0.0f;

void sh_shots_on_attack(void)
{
    f32 now = g_pmove ? g_pmove->time : 0.0f;

    if (now - g_last_attack_time > 0.35f)
        g_shots_fired = 0;

    g_last_attack_time = now;

    if (g_shots_fired < 40)
        g_shots_fired++;
}

void sh_shots_reset(void)
{
    g_shots_fired = 0;
    g_last_attack_time = 0.0f;
}

i32 sh_shots_count(void)
{
    return g_shots_fired;
}

i32 sh_local_weapon_id(void)
{
    sh_cl_entity_t *local = sh_get_entity(g_local_player);

    if (!local)
        return 0;

    return local->curstate.weaponmodel;
}

void sh_movement_run(sh_usercmd_t *cmd)
{
    sh_playermove_t *pm = g_pmove;

    if (!pm || !cmd)
        return;

    if (g_cfg.movement.autostrafe)
        sh_autostrafe(cmd, pm);
    else if (g_cfg.movement.quick_stop)
        sh_quick_stop(cmd, pm);

    sh_bhop(cmd, pm);
    sh_auto_jump_hold(cmd, pm);

    if (cmd->buttons & 1)
        sh_shots_on_attack();
    else
        sh_shots_reset();

    if (g_cfg.misc.no_recoil)
        sh_recoil_compensate(&cmd->viewangles, sh_local_weapon_id(), g_shots_fired);
}
