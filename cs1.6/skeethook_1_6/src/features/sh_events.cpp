#include "../../include/sh_features.h"
#include "../../include/sh_visuals.h"
#include "../../include/sh_ballistics.h"
#include "../../include/sh_math.h"
#include "../../include/sh_config.h"

static f32 g_last_fire_time[33];
static i32 g_fire_count[33];

void sh_events_reset(void)
{
    for (i32 i = 0; i < 33; ++i)
    {
        g_last_fire_time[i] = 0.0f;
        g_fire_count[i] = 0;
    }
}

void sh_events_on_fire(i32 entindex, const vec3_t *origin, i32 event_id)
{
    if (entindex <= 0 || entindex > 32)
        return;

    if (!origin)
        return;

    f32 now = g_pmove ? g_pmove->time : 0.0f;

    if (now - g_last_fire_time[entindex] > 0.12f)
        g_fire_count[entindex] = 0;

    ++g_fire_count[entindex];
    g_last_fire_time[entindex] = now;

    sh_player_t *shooter = sh_player_get(entindex);

    if (g_cfg.visuals.tracer && shooter && shooter->ent)
    {
        vec3_t from = *origin;
        vec3_t ang = shooter->ent->angles;

        vec3_t fwd, rightv, upv;
        sh_angle_vectors(&ang, &fwd, &rightv, &upv);

        vec3_t to;
        to.x = from.x + fwd.x * 8192.0f;
        to.y = from.y + fwd.y * 8192.0f;
        to.z = from.z + fwd.z * 8192.0f;

        vec3_t end;

        if (sh_trace_line(&from, &to, &end))
            to = end;

        sh_visuals_tracer_add(&from, &to);
    }

    (void)event_id;
}

i32 sh_events_fire_count(i32 entindex)
{
    if (entindex <= 0 || entindex > 32)
        return 0;

    return g_fire_count[entindex];
}

f32 sh_events_last_fire(i32 entindex)
{
    if (entindex <= 0 || entindex > 32)
        return 0.0f;

    return g_last_fire_time[entindex];
}

qbool sh_events_is_firing(i32 entindex, f32 window)
{
    if (entindex <= 0 || entindex > 32)
        return 0;

    f32 now = g_pmove ? g_pmove->time : 0.0f;

    return (now - g_last_fire_time[entindex]) <= window;
}

void sh_misc_on_post_run(sh_usercmd_t *cmd, i32 runfuncs, f64 time)
{
    (void)cmd;
    (void)runfuncs;
    (void)time;
}
