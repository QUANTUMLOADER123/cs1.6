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



static void sh_edge_jump(sh_usercmd_t *cmd)
{
    if (!g_pmove || !g_pmove->movevars)
        return;

    if (!(g_pmove->flags & sh_fl_onground))
        return;

    if (g_pmove->waterlevel > sh_waterlevel_none)
        return;

    if (!g_trace_fn)
        return;

    f32 vx = g_pmove->velocity.x;
    f32 vy = g_pmove->velocity.y;

    f32 speed = sh_sqrt(vx * vx + vy * vy);

    if (speed < 100.0f)
        return;

    f32 dirx = vx / speed;
    f32 diry = vy / speed;

    f32 ft = g_pmove->frametime;

    if (ft <= 0.0f)
        ft = 0.01f;

    if (ft > 0.1f)
        ft = 0.1f;

    f32 per_frame = speed * ft;

    f32 probe = per_frame * 1.5f;

    if (probe < 4.0f)
        probe = 4.0f;
    if (probe > 48.0f)
        probe = 48.0f;

    vec3_t ahead;
    ahead.x = g_pmove->origin.x + dirx * probe;
    ahead.y = g_pmove->origin.y + diry * probe;
    ahead.z = g_pmove->origin.z;

    vec3_t down = ahead;
    down.z -= 48.0f;

    i32 hit = g_trace_fn(ahead, down, 0, -1);

    if (hit)
        return;

    cmd->buttons |= sh_in_jump;
}

static void sh_fast_ladder(sh_usercmd_t *cmd)
{
    if (!g_pmove)
        return;

    if (g_pmove->movetype != sh_movetype_fly)
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


extern sh_weapon_data_t g_weapon_data[sh_max_weapons];

static void sh_fast_reload(sh_usercmd_t *cmd)
{
    if (!g_pmove)
        return;

    sh_player_t *local = sh_player_local();

    if (!local)
        return;

    i32 id = local->weapon_id;

    if (id <= 0 || id >= sh_max_weapons)
        return;

    const sh_weapon_data_t *w = &g_weapon_data[id];

    if (w->clip != 0)
        return;

    if (w->in_reload)
        return;

    f32 now = g_pmove->time;

    if (w->next_primary_attack > now)
        return;

    cmd->buttons |= sh_in_reload;
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
