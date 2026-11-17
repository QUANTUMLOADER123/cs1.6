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
static sh_impact_t g_impacts[sh_max_impacts];
static i32 g_impact_cursor;
static i32 g_saved_rendermode = -1;
static f32 g_saved_monolights;
static f32 g_saved_overbright;
static f32 g_saved_fog;
static f32 g_saved_lightholes;
static f32 g_saved_flashblend;

static f32 sh_get_cvar(const char *name)
{
    if (!g_engfuncs || !g_engfuncs->pfnGetCvarPointer)
        return 0.0f;

    void *cv = g_engfuncs->pfnGetCvarPointer(name);

    if (!cv)
        return 0.0f;

    return *(f32 *)((u8 *)cv + 12);
}

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
    if (!pos)
        return;

    sh_impact_t *im = &g_impacts[g_impact_cursor];

    im->pos = *pos;

    if (normal)
        im->normal = *normal;
    else
        im->normal = (vec3_t){ 0.0f, 0.0f, 1.0f };

    im->born = sh_now();
    im->life = sh_impact_life;
    im->active = 1;

    g_impact_cursor = (g_impact_cursor + 1) % sh_max_impacts;

    if (g_cfg.visuals.bullet_impact_sound && g_engfuncs && g_engfuncs->pfnPlaySoundByName)
        g_engfuncs->pfnPlaySoundByName("weapons/bullet_hit1.wav", 0.4f);
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

    for (i32 i = 0; i < sh_max_impacts; ++i)
    {
        if (!g_impacts[i].active)
            continue;

        if (now - g_impacts[i].born > g_impacts[i].life)
            g_impacts[i].active = 0;
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

    if (!g_cfg.visuals.impact)
        return;

    for (i32 i = 0; i < sh_max_impacts; ++i)
    {
        sh_impact_t *im = &g_impacts[i];

        if (!im->active)
            continue;

        vec2_t sp;

        if (!sh_world_to_screen(&im->pos, &sp))
            continue;

        f32 age = (sh_now() - im->born) / im->life;
        f32 fade = 1.0f - sh_clampf(age, 0.0f, 1.0f);

        col4_t c = g_cfg.visuals.col_impact;
        c.a = (u8)((f32)c.a * fade);

        f32 r = (2.0f + 4.0f * age) * sh_scale();

        sh_draw_line(sp.x - r, sp.y, sp.x + r, sp.y, 1.0f, c);
        sh_draw_line(sp.x, sp.y - r, sp.x, sp.y + r, 1.0f, c);
    }
}

void sh_visuals_apply_nightmode(void)
{
    if (!g_engfuncs || !g_engfuncs->pfnCvar_SetValue)
        return;

    if (!g_cfg.visuals.nightmode)
    {
        if (g_saved_rendermode >= 0)
            sh_visuals_restore();
        return;
    }

    col4_t w = g_cfg.visuals.col_world;
    col4_t s = g_cfg.visuals.col_sky;
    col4_t l = g_cfg.visuals.col_light;

    f32 wr = (f32)w.r / 255.0f;
    f32 wg = (f32)w.g / 255.0f;
    f32 wb = (f32)w.b / 255.0f;

    f32 sr = (f32)s.r / 255.0f;
    f32 sg = (f32)s.g / 255.0f;
    f32 sb = (f32)s.b / 255.0f;

    f32 lr = (f32)l.r / 255.0f;
    f32 lg = (f32)l.g / 255.0f;
    f32 lb = (f32)l.b / 255.0f;

    if (g_saved_rendermode < 0)
    {
        g_saved_rendermode = 1;
        g_saved_monolights = sh_get_cvar("gl_monolights");
        g_saved_overbright = sh_get_cvar("gl_overbright");
        g_saved_fog = sh_get_cvar("gl_fog");
        g_saved_lightholes = sh_get_cvar("gl_lightholes");
        g_saved_flashblend = sh_get_cvar("gl_flashblend");
    }

    g_engfuncs->pfnCvar_SetValue("gl_monolights", (wr + wg + wb) / 3.0f);
    g_engfuncs->pfnCvar_SetValue("gl_overbright", (lr + lg + lb) / 3.0f);
    g_engfuncs->pfnCvar_SetValue("gl_fog", (f32)(g_cfg.visuals.world_color ? 1 : 0));
    g_engfuncs->pfnCvar_SetValue("gl_lightholes", sr);
    g_engfuncs->pfnCvar_SetValue("gl_flashblend", sg);
}

void sh_visuals_restore(void)
{
    if (g_saved_rendermode < 0)
        return;

    if (g_engfuncs && g_engfuncs->pfnCvar_SetValue)
    {
        g_engfuncs->pfnCvar_SetValue("gl_monolights", g_saved_monolights);
        g_engfuncs->pfnCvar_SetValue("gl_overbright", g_saved_overbright);
        g_engfuncs->pfnCvar_SetValue("gl_fog", g_saved_fog);
        g_engfuncs->pfnCvar_SetValue("gl_lightholes", g_saved_lightholes);
        g_engfuncs->pfnCvar_SetValue("gl_flashblend", g_saved_flashblend);
    }

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
