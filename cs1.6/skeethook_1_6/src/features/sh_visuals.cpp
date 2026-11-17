#include "../../include/sh_visuals.h"
#include "../../include/sh_config.h"
#include "../../include/sh_render.h"
#include "../../include/sh_math.h"

typedef struct sh_tracer_s
{
    vec3_t from;
    vec3_t to;
    f32 born;
    f32 life;
    qbool active;
} sh_tracer_t;

static sh_tracer_t g_tracers[sh_max_tracers];
static i32 g_tracer_cursor;
static i32 g_saved_rendermode = -1;

static f32 sh_now(void)
{
    return g_pmove ? g_pmove->time : 0.0f;
}

void sh_visuals_tracer_add(const vec3_t *from, const vec3_t *to)
{
    if (!from || !to)
        return;

    sh_tracer_t *t = &g_tracers[g_tracer_cursor];

    t->from = *from;
    t->to = *to;
    t->born = sh_now();
    t->life = sh_tracer_life;
    t->active = 1;

    g_tracer_cursor = (g_tracer_cursor + 1) % sh_max_tracers;
}

void sh_visuals_impact_add(const vec3_t *pos, const vec3_t *normal)
{
    if (!pos || !normal)
        return;

    if (!g_tri_api || !g_tri_api->box_in_pvs)
        return;

    vec3_t mins = { pos->x - 1.0f, pos->y - 1.0f, pos->z - 1.0f };
    vec3_t maxs = { pos->x + 1.0f, pos->y + 1.0f, pos->z + 1.0f };

    (void)g_tri_api->box_in_pvs((f32 *)&mins, (f32 *)&maxs);
    (void)normal;
}

void sh_visuals_effects_tick(f32 frametime)
{
    f32 now = sh_now();

    for (i32 i = 0; i < sh_max_tracers; ++i)
    {
        if (!g_tracers[i].active)
            continue;

        if (now - g_tracers[i].born > g_tracers[i].life)
            g_tracers[i].active = 0;
    }
}

void sh_visuals_effects_draw(void)
{
    if (!g_cfg.visuals.tracer)
        return;

    for (i32 i = 0; i < sh_max_tracers; ++i)
    {
        sh_tracer_t *t = &g_tracers[i];

        if (!t->active)
            continue;

        vec2_t a, b;

        if (!sh_world_to_screen(&t->from, &a))
            continue;
        if (!sh_world_to_screen(&t->to, &b))
            continue;

        f32 age = (sh_now() - t->born) / t->life;
        f32 fade = 1.0f - sh_clampf(age, 0.0f, 1.0f);

        col4_t c = g_cfg.visuals.col_tracer;
        c.a = (u8)((f32)c.a * fade);

        sh_draw_line(a.x, a.y, b.x, b.y, 1.0f, c);
    }
}

void sh_visuals_apply_nightmode(void)
{
    if (!g_engfuncs)
        return;

    if (g_cfg.visuals.nightmode && g_saved_rendermode < 0)
        g_saved_rendermode = 0;

    if (!g_cfg.visuals.nightmode)
        return;

    col4_t w = g_cfg.visuals.col_world;
    col4_t s = g_cfg.visuals.col_sky;
    col4_t l = g_cfg.visuals.col_light;

    f32 wr = (f32)w.r / 255.0f, wg = (f32)w.g / 255.0f, wb = (f32)w.b / 255.0f;
    f32 sr = (f32)s.r / 255.0f, sg = (f32)s.g / 255.0f, sb = (f32)s.b / 255.0f;
    f32 lr = (f32)l.r / 255.0f, lg = (f32)l.g / 255.0f, lb = (f32)l.b / 255.0f;

    if (g_engfuncs->pfnCvar_SetValue)
    {
        if (g_engfuncs->pfnCvar_SetValue) g_engfuncs->pfnCvar_SetValue("r_reflect", wb);
        if (g_engfuncs->pfnCvar_SetValue) g_engfuncs->pfnCvar_SetValue("gl_skycolor_r", sr);
        if (g_engfuncs->pfnCvar_SetValue) g_engfuncs->pfnCvar_SetValue("gl_skycolor_g", sg);
        if (g_engfuncs->pfnCvar_SetValue) g_engfuncs->pfnCvar_SetValue("gl_skycolor_b", sb);
        if (g_engfuncs->pfnCvar_SetValue) g_engfuncs->pfnCvar_SetValue("gl_world_light", (wr + wg + wb) / 3.0f);
        if (g_engfuncs->pfnCvar_SetValue) g_engfuncs->pfnCvar_SetValue("gl_light", (lr + lg + lb) / 3.0f);
    }
}

void sh_visuals_restore(void)
{
    if (g_saved_rendermode < 0)
        return;

    g_saved_rendermode = -1;
}

void sh_visuals_thirdperson(sh_usercmd_t *cmd)
{
    if (!g_cfg.visuals.thirdperson || !cmd)
        return;

    vec3_t fwd, right, up;
    sh_angle_vectors(&cmd->viewangles, &fwd, &right, &up);

    f32 dist = g_cfg.visuals.thirdperson_dist;

    if (dist < 32.0f)
        dist = 32.0f;

    (void)fwd;
    (void)right;
    (void)up;
    (void)dist;

    if (g_engfuncs && g_engfuncs->pfnSetViewAngles)
        g_engfuncs->pfnSetViewAngles((f32 *)&cmd->viewangles);
}
