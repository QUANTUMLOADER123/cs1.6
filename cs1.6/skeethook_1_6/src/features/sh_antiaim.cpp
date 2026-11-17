#include "../../include/sh_antiaim.h"
#include "../../include/sh_config.h"
#include "../../include/sh_math.h"

static f32 g_prev_time;
static f32 g_spin_accum;
static qbool g_jitter_flip;
static qbool g_desync_flip;
static qbool g_sideways_flip;
static u32 g_jitter_seed = 12345u;

static u32 sh_rand(void)
{
    g_jitter_seed = g_jitter_seed * 1103515245u + 12345u;
    return (g_jitter_seed >> 16) & 0x7FFF;
}

static i32 sh_rand_range(i32 lo, i32 hi)
{
    if (hi <= lo)
        return lo;

    return lo + (i32)(sh_rand() % (u32)(hi - lo + 1));
}

f32 sh_antiaim_max_delta(void)
{
    return 91.0f;
}

qbool sh_antiaim_can_run(sh_usercmd_t *cmd)
{
    if (!cmd)
        return 0;

    if (cmd->buttons & 1)
        return 0;

    if (cmd->buttons & 32)
        return 0;

    if (!g_pmove)
        return 0;

    if (g_pmove->movetype == 5)
        return 0;

    if (g_pmove->movetype == 6 && g_pmove->onground != -1)
        return 0;

    return 1;
}

static f32 sh_time_flow(f32 time)
{
    f32 dt = time - g_prev_time;

    if (dt < 0.0f || dt > 1.0f)
        dt = 0.0f;

    g_prev_time = time;

    return dt;
}

void sh_antiaim_apply_pitch(sh_usercmd_t *cmd, f32 time)
{
    switch (g_cfg.antiaim.pitch_mode)
    {
    case sh_pitch_down:
        cmd->viewangles.x = 88.0f;
        break;
    case sh_pitch_up:
        cmd->viewangles.x = -88.0f;
        break;
    case sh_pitch_fake_down:
        cmd->viewangles.x = 1620.0f;
        break;
    case sh_pitch_fake_up:
        cmd->viewangles.x = -1619.0f;
        break;
    case sh_pitch_spin:
        cmd->viewangles.x = sh_cos(time * 11.0f * sh_pi) * 90.0f;
        break;
    case sh_pitch_static:
        cmd->viewangles.x = 0.0f;
        break;
    default:
        break;
    }
}

void sh_antiaim_apply_yaw(sh_usercmd_t *cmd, f32 time)
{
    f32 base = cmd->viewangles.y;

    switch (g_cfg.antiaim.yaw_mode)
    {
    case sh_yaw_backwards:
        cmd->viewangles.y = base + 180.0f;
        break;
    case sh_yaw_static_left:
        cmd->viewangles.y = 90.0f;
        break;
    case sh_yaw_static_right:
        cmd->viewangles.y = 270.0f;
        break;
    case sh_yaw_left:
        cmd->viewangles.y = base + 90.0f;
        break;
    case sh_yaw_right:
        cmd->viewangles.y = base + 270.0f;
        break;
    case sh_yaw_spin:
        g_spin_accum += g_cfg.antiaim.spin_speed * g_cfg.antiaim.spin_speed;
        cmd->viewangles.y = base + g_spin_accum;
        break;
    case sh_yaw_static:
        cmd->viewangles.y = 0.0f;
        break;
    case sh_yaw_random:
    {
        i32 r = sh_rand_range(0, 99);

        if (r < 98)
            cmd->viewangles.y = base - 180.0f;

        if (r < 15)
            cmd->viewangles.y += (f32)sh_rand_range(-70, 70);

        if (r == 69)
            cmd->viewangles.y += (f32)sh_rand_range(-90, 90);

        break;
    }
    case sh_yaw_jitter:
    {
        f32 jitter = g_cfg.antiaim.yaw_jitter;

        if (jitter <= 0.0f)
            jitter = 35.0f;

        if (jitter > 90.0f)
            jitter = 90.0f;

        cmd->viewangles.y = base + ((g_jitter_flip) ? jitter : -jitter);
        g_jitter_flip = !g_jitter_flip;

        break;
    }
    case sh_yaw_fake_sideways:
        cmd->viewangles.y = base - sh_antiaim_max_delta();

        g_sideways_flip = !g_sideways_flip;

        if (g_sideways_flip)
            cmd->viewangles.y -= 180.0f;

        break;
    default:
        break;
    }

    if (g_cfg.antiaim.roll != 0.0f)
        cmd->viewangles.z = g_cfg.antiaim.roll;
}

void sh_antiaim_movement_fix(sh_usercmd_t *cmd, const vec3_t *old_angles)
{
    if (!cmd || !old_angles)
        return;

    vec3_t view_fwd, view_right, view_up;
    vec3_t aim_fwd, aim_right, aim_up;

    sh_angle_vectors(old_angles, &view_fwd, &view_right, &view_up);

    vec3_t flat = cmd->viewangles;
    flat.x = 0.0f;
    flat.z = 0.0f;

    sh_angle_vectors(&flat, &aim_fwd, &aim_right, &aim_up);

    f32 fwd = cmd->forwardmove;
    f32 side = cmd->sidemove;
    f32 up = cmd->upmove;

    cmd->forwardmove = fwd * sh_dot(&view_fwd, &aim_fwd)
                     + side * sh_dot(&view_right, &aim_fwd)
                     + up * sh_dot(&view_up, &aim_fwd);

    cmd->sidemove = fwd * sh_dot(&view_fwd, &aim_right)
                  + side * sh_dot(&view_right, &aim_right)
                  + up * sh_dot(&view_up, &aim_right);

    cmd->upmove = fwd * sh_dot(&view_fwd, &aim_up)
                + side * sh_dot(&view_right, &aim_up)
                + up * sh_dot(&view_up, &aim_up);
}

void sh_antiaim_run(sh_usercmd_t *cmd)
{
    if (!cmd || !g_cfg.antiaim.enabled)
        return;

    if (!sh_antiaim_can_run(cmd))
        return;

    f32 time = g_pmove ? g_pmove->time : 0.0f;
    sh_time_flow(time);

    vec3_t old_angles = cmd->viewangles;

    sh_antiaim_apply_pitch(cmd, time);
    sh_antiaim_apply_yaw(cmd, time);

    if (g_cfg.antiaim.desync)
    {
        f32 amount = g_cfg.antiaim.desync_amount;

        if (amount <= 0.0f)
            amount = 60.0f;

        if (amount > 90.0f)
            amount = 90.0f;

        cmd->viewangles.y += (g_desync_flip) ? amount : -amount;
        g_desync_flip = !g_desync_flip;
    }

    if (g_cfg.antiaim.fake_duck && g_pmove && g_pmove->onground != -1)
        cmd->buttons |= 4;

    if (g_cfg.antiaim.micro_move && g_pmove)
    {
        f32 speed = g_cfg.antiaim.micro_move_speed;

        if (speed <= 0.0f)
            speed = 1.5f;

        cmd->forwardmove = speed;
    }

    sh_antiaim_movement_fix(cmd, &old_angles);
}
