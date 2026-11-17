#include "../../include/sh_features.h"
#include "../../include/sh_math.h"

#include <windows.h>
#include <math.h>

enum
{
    sh_pitch_none = 0,
    sh_pitch_down,
    sh_pitch_up,
    sh_pitch_fake_down,
    sh_pitch_fake_up,
    sh_pitch_random
};

enum
{
    sh_yaw_none = 0,
    sh_yaw_backward,
    sh_yaw_spin,
    sh_yaw_jitter,
    sh_yaw_fake_side,
    sh_yaw_lby_break
};

static f32 g_spin_angle;
static f32 g_jitter_phase;
static f32 g_lby_timer;
static f32 g_fake_duck_timer;

static f32 sh_get_frametime(void)
{
    if (g_pmove && g_pmove->frametime > 0.0f)
        return g_pmove->frametime;
    return 0.015f;
}

static f32 sh_randf(f32 lo, f32 hi)
{
    static u32 seed = 0x9E3779B9;
    seed ^= seed << 13;
    seed ^= seed >> 17;
    seed ^= seed << 5;
    f32 t = (f32)(seed & 0xFFFF) / 65535.0f;
    return lo + t * (hi - lo);
}

void sh_antiaim_apply_pitch(vec3_t *angles, sh_usercmd_t *cmd, f32 frametime)
{
    (void)cmd;
    (void)frametime;

    switch (g_cfg.antiaim.pitch_mode)
    {
    case sh_pitch_down:
        angles->x = 89.0f;
        break;
    case sh_pitch_up:
        angles->x = -89.0f;
        break;
    case sh_pitch_fake_down:
        angles->x = 89.0f;
        break;
    case sh_pitch_fake_up:
        angles->x = -89.0f;
        break;
    case sh_pitch_random:
        angles->x = sh_randf(-89.0f, 89.0f);
        break;
    default:
        break;
    }
}

void sh_antiaim_apply_yaw(vec3_t *angles, sh_usercmd_t *cmd, f32 frametime)
{
    f32 base = g_cfg.antiaim.yaw_base;

    switch (g_cfg.antiaim.yaw_mode)
    {
    case sh_yaw_backward:
        angles->y += 180.0f;
        break;

    case sh_yaw_spin:
        g_spin_angle += g_cfg.antiaim.spin_speed * frametime * 360.0f;
        while (g_spin_angle > 360.0f)
            g_spin_angle -= 360.0f;
        angles->y += g_spin_angle;
        break;

    case sh_yaw_jitter:
        g_jitter_phase += frametime * 8.0f;
        angles->y += sinf(g_jitter_phase) * g_cfg.antiaim.yaw_jitter;
        break;

    case sh_yaw_fake_side:
        angles->y += g_cfg.antiaim.yaw_jitter * ((cmd->buttons & sh_in_duck) ? 1.0f : -1.0f);
        break;

    case sh_yaw_lby_break:
        g_lby_timer += frametime;
        if (g_lby_timer > 1.1f)
        {
            angles->y += 180.0f;
            g_lby_timer = 0.0f;
        }
        break;

    default:
        break;
    }

    angles->y += base;
    sh_angle_normalize(angles);
}

static void sh_micro_move(sh_usercmd_t *cmd)
{
    f32 speed = g_cfg.antiaim.micro_move_speed;
    if (speed <= 0.0f)
        return;

    i32 phase = (i32)(g_spin_angle * 10.0f) % 4;

    switch (phase)
    {
    case 0:
        cmd->forwardmove = speed;
        cmd->sidemove = 0.0f;
        break;
    case 1:
        cmd->forwardmove = 0.0f;
        cmd->sidemove = speed;
        break;
    case 2:
        cmd->forwardmove = -speed;
        cmd->sidemove = 0.0f;
        break;
    default:
        cmd->forwardmove = 0.0f;
        cmd->sidemove = -speed;
        break;
    }
}

void sh_antiaim_run(sh_usercmd_t *cmd)
{
    if (!g_cfg.antiaim.enabled || !cmd)
        return;

    f32 ft = sh_get_frametime();

    if (g_cfg.antiaim.pitch_mode)
        sh_antiaim_apply_pitch(&cmd->viewangles, cmd, ft);

    if (g_cfg.antiaim.yaw_mode)
        sh_antiaim_apply_yaw(&cmd->viewangles, cmd, ft);

    if (g_cfg.antiaim.micro_move)
        sh_micro_move(cmd);

    if (g_cfg.antiaim.fake_duck)
    {
        g_fake_duck_timer += ft;
        if (g_fake_duck_timer > 0.2f)
        {
            cmd->buttons |= sh_in_duck;
            g_fake_duck_timer = 0.0f;
        }
        else
        {
            cmd->buttons &= ~sh_in_duck;
        }
    }
}
