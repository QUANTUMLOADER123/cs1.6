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

    if (wish_speed < 1.0f)
        return;

    f32 omega = sh_strafe_omega(wish_speed);

    f32 vel_yaw = sh_atan2_deg(vy, vx);

    f32 fx = sh_cos_deg(cmd->viewangles.y);
    f32 fy = sh_sin_deg(cmd->viewangles.y);

    f32 d_right = fy * (vx / speed) - fx * (vy / speed);
    (void)fx;
    (void)fy;

    f32 target_yaw = vel_yaw + omega;

    if (d_right < 0.0f)
        target_yaw = vel_yaw - omega;

    f32 f = sh_cos_deg(target_yaw - cmd->viewangles.y);
    f32 r = sh_sin_deg(target_yaw - cmd->viewangles.y);

    cmd->forwardmove = f * sh_max_move;
    cmd->sidemove = -r * sh_max_move;
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

    if (speed < pm->maxspeed * 0.25f)
        return;

    f32 vel_yaw = sh_atan2_deg(vy, vx);
    f32 back = vel_yaw + 180.0f;

    f32 dw = back - cmd->viewangles.y;

    while (dw > 180.0f)
        dw -= 360.0f;
    while (dw < -180.0f)
        dw += 360.0f;

    f32 r = sh_sin_deg(dw);

    cmd->forwardmove = (sh_cos_deg(dw) < 0.0f) ? -sh_max_move : sh_max_move;
    cmd->sidemove = -r * sh_max_move;

    (void)vel_yaw;
}

void sh_bhop(sh_usercmd_t *cmd, sh_playermove_t *pm)
{
    if (!g_cfg.movement.bhop || !cmd || !pm)
        return;

    if (pm->onground == -1)
        return;

    static i32 g_drop_frame;

    if (g_drop_frame)
    {
        cmd->buttons &= ~2;
        g_drop_frame = 0;
        return;
    }

    if ((pm->oldbuttons & 2) && (cmd->buttons & 2))
    {
        cmd->buttons &= ~2;
        g_drop_frame = 1;
        return;
    }

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

void sh_super_toss(sh_usercmd_t *cmd, sh_playermove_t *pm)
{
    if (!g_cfg.movement.super_toss || !cmd || !pm)
        return;

    if (!(cmd->buttons & 2))
        return;

    f32 pitch = cmd->viewangles.x;

    f32 ang = pitch;

    if (ang < 0.0f)
        ang = -10.0f + ang * (80.0f / 90.0f);
    else
        ang = -10.0f + ang * (100.0f / 90.0f);

    f32 vel = (90.0f - ang) * 6.0f;

    if (vel >= 750.0f)
        return;

    f32 want = g_cfg.movement.super_toss_power;

    if (want <= 0.0f)
        want = 100.0f;

    if (want > 100.0f)
        want = 100.0f;

    f32 target_vel = 540.0f + (750.0f - 540.0f) * (want / 100.0f);
    f32 want_ang = 90.0f - target_vel / 6.0f;

    f32 want_pitch;

    if (want_ang < -10.0f)
        want_pitch = (want_ang + 10.0f) / (80.0f / 90.0f);
    else
        want_pitch = (want_ang + 10.0f) / (100.0f / 90.0f);

    if (want_pitch < -89.0f)
        want_pitch = -89.0f;

    cmd->viewangles.x = want_pitch;
}

void sh_tick_control(sh_usercmd_t *cmd)
{
    if (!g_cfg.movement.tick_control || !cmd || !g_pmove)
        return;

    f32 ft = g_pmove->frametime;

    if (ft <= 0.0f)
        return;

    f32 target = g_cfg.movement.tick_ms;

    if (target < 1.0f)
        target = 1.0f;
    if (target > 100.0f)
        target = 100.0f;

    cmd->msec = (u8)target;
}

void sh_movement_run(sh_usercmd_t *cmd)
{
    sh_playermove_t *pm = g_pmove;

    if (!pm || !cmd)
        return;

    if (g_cfg.movement.autostrafe)
        sh_autostrafe(cmd, pm);

    sh_quick_stop(cmd, pm);

    sh_tick_control(cmd);

    sh_bhop(cmd, pm);
    sh_auto_jump_hold(cmd, pm);
    sh_super_toss(cmd, pm);

    if (cmd->buttons & 1)
        sh_shots_on_attack();
    else
        sh_shots_reset();

    if (g_cfg.misc.no_recoil)
        sh_recoil_compensate(&cmd->viewangles, sh_local_weapon_id(), g_shots_fired);
}

static f32 g_fall_damage = 0.0f;
static f32 g_fall_show_until = 0.0f;

f32 sh_fall_damage_predicted(void)
{
    return g_fall_damage;
}

void sh_fall_predict(sh_playermove_t *pm)
{
    if (!pm)
        return;

    if (pm->onground == -1)
    {
        f32 v = pm->fall_velocity;

        if (v > 500.0f)
        {
            g_fall_damage = (v - 500.0f) * (100.0f / 600.0f);

            if (pm->waterlevel > 0 || pm->waterjumptime > 0.0f)
                g_fall_damage = 0.0f;
        }
        else
        {
            g_fall_damage = 0.0f;
        }

        return;
    }

    if (g_fall_damage > 0.0f)
        g_fall_show_until = pm->time + 2.0f;

    g_fall_damage = 0.0f;
}

qbool sh_fall_warning(f32 *damage)
{
    if (g_fall_damage > 0.0f)
    {
        if (damage)
            *damage = g_fall_damage;

        return 1;
    }

    return 0;
}
