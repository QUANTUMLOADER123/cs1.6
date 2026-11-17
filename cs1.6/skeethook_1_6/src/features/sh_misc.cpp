#include "../../include/sh_knife.h"
#include "../../include/sh_features.h"
#include "../../include/sh_math.h"

#include <windows.h>
#include <math.h>

static qbool g_jump_held;

static i32 g_crosshair_weapon;
static f32 g_next_reload;

#define sh_waterlevel_none 0
#define sh_waterlevel_feet 1

static f32 sh_frametime(void)
{
    if (g_pmove && g_pmove->frametime > 0.0f)
        return g_pmove->frametime;
    return 0.015f;
}

static void sh_bhop(sh_usercmd_t *cmd)
{
    if (!g_pmove)
        return;

    if (!(g_pmove->flags & sh_fl_onground))
    {
        cmd->buttons |= sh_in_jump;
        return;
    }

    if (g_jump_held)
        cmd->buttons &= ~sh_in_jump;
    else
        cmd->buttons |= sh_in_jump;

    g_jump_held = !g_jump_held;
}


static void sh_edge_jump(sh_usercmd_t *cmd)
{
    if (!g_pmove || !g_pmove->movevars)
        return;

    if (!(g_pmove->flags & sh_fl_onground))
        return;

    if (g_pmove->waterlevel > sh_waterlevel_none)
        return;

    vec3_t start = g_pmove->origin;
    vec3_t ahead;
    sh_scale(&ahead, &g_pmove->velocity, 0.1f);
    sh_add(&ahead, &start, &ahead);

    vec3_t down = ahead;
    down.z -= 48.0f;

    if (!g_trace_fn)
        return;

    i32 hit = g_trace_fn(ahead, down, 0, -1);
    if (!hit)
        cmd->buttons |= sh_in_jump;
}

static void sh_fast_ladder(sh_usercmd_t *cmd)
{
    if (!g_pmove)
        return;

    if (g_pmove->movetype != 5)
        return;

    vec3_t fwd;
    sh_angle_vectors(&cmd->viewangles, &fwd, 0, 0);
    fwd.z = 0.0f;
    sh_normalize(&fwd);
    sh_scale(&fwd, &fwd, 200.0f);

    cmd->forwardmove = fwd.x;
    cmd->sidemove = fwd.y;
    cmd->upmove = 0.0f;
}

static void sh_auto_pistol(sh_usercmd_t *cmd)
{
    if (!g_pmove)
        return;

    f32 now = g_pmove->time;
    f32 margin = g_pmove->frametime * 0.5f;

    if (g_pmove->next_primary_attack > now + margin)
        return;

    cmd->buttons |= sh_in_attack;
}


static void sh_fast_reload(sh_usercmd_t *cmd)
{
    f32 ft = sh_frametime();
    g_next_reload -= ft;
    if (g_next_reload > 0.0f)
        return;

    cmd->buttons |= sh_in_reload;
    g_next_reload = 0.1f;
}

void sh_misc_run(sh_usercmd_t *cmd)
{
    if (!cmd)
        return;

    if (g_cfg.misc.edge_jump)
        sh_edge_jump(cmd);

    if (g_cfg.misc.fast_ladder)
        sh_fast_ladder(cmd);

    if (g_cfg.misc.auto_pistol)
        sh_auto_pistol(cmd);

    if (g_cfg.misc.fast_reload)
        sh_fast_reload(cmd);

    if (g_cfg.misc.knife_bot)
        sh_knife_run(cmd);

    if (g_cfg.misc.no_scope && (cmd->buttons & sh_in_attack2))
        cmd->buttons &= ~sh_in_attack2;
}

void sh_misc_screen_init(void)
{
    g_crosshair_weapon = 0;
    g_next_reload = 0.0f;
}
