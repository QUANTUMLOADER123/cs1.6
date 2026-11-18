#include "../../include/sh_visuals.h"
#include "../../include/sh_config.h"
#include "../../include/sh_render.h"
#include "../../include/sh_math.h"
#include "../../include/sh_grenade.h"
#include "../../include/sh_features.h"

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
static i32 g_nightmode_saved = -1;
static f32 g_saved_monolights;
static f32 g_saved_overbright;
static f32 g_saved_fog;
static f32 g_saved_lightholes;
static f32 g_saved_flashblend;
static f32 g_saved_lightgamma;
static f32 g_saved_texgamma;
static f32 g_saved_gamma;
static f32 g_saved_brightness;

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
    for (i32 i = 0; g_cfg.visuals.tracer && i < sh_max_tracers; ++i)
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

    for (i32 i = 0; g_cfg.visuals.impact && i < sh_max_impacts; ++i)
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
        if (g_nightmode_saved >= 0)
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

    if (g_nightmode_saved < 0)
    {
        g_nightmode_saved = 1;
        g_saved_monolights = sh_get_cvar("gl_monolights");
        g_saved_overbright = sh_get_cvar("gl_overbright");
        g_saved_fog = sh_get_cvar("gl_fog");
        g_saved_lightholes = sh_get_cvar("gl_lightholes");
        g_saved_flashblend = sh_get_cvar("gl_flashblend");
        g_saved_lightgamma = sh_get_cvar("lightgamma");
        g_saved_texgamma = sh_get_cvar("texgamma");
        g_saved_gamma = sh_get_cvar("gamma");
        g_saved_brightness = sh_get_cvar("brightness");
    }

    if (g_cfg.visuals.world_color)
    {
        f32 lum = wr * 0.299f + wg * 0.587f + wb * 0.114f;

        if (lum < 0.0f) lum = 0.0f;
        if (lum > 1.0f) lum = 1.0f;

        g_engfuncs->pfnCvar_SetValue("gl_monolights", 1.0f);
        g_engfuncs->pfnCvar_SetValue("lightgamma", 2.5f - lum * 1.5f);
        g_engfuncs->pfnCvar_SetValue("texgamma", 2.5f - lum * 1.5f);
        g_engfuncs->pfnCvar_SetValue("gl_lightholes", 0.0f);
    }

    if (g_cfg.visuals.sky_color)
    {
        f32 avg = (sr + sg + sb) / 3.0f;

        if (avg < 0.0f) avg = 0.0f;
        if (avg > 1.0f) avg = 1.0f;

        g_engfuncs->pfnCvar_SetValue("gamma", 3.0f - avg * 1.5f);
        g_engfuncs->pfnCvar_SetValue("brightness", 2.0f - avg * 1.2f);
    }

    if (g_cfg.visuals.light_color)
    {
        f32 avg = (lr + lg + lb) / 3.0f;

        if (avg < 0.0f) avg = 0.0f;
        if (avg > 1.0f) avg = 1.0f;

        g_engfuncs->pfnCvar_SetValue("gl_fog", (avg > 0.5f) ? 0.0f : 1.0f);
        g_engfuncs->pfnCvar_SetValue("gl_flashblend", (avg > 0.5f) ? 1.0f : 0.0f);
    }
}

void sh_visuals_restore(void)
{
    if (g_nightmode_saved < 0)
        return;

    if (g_engfuncs && g_engfuncs->pfnCvar_SetValue)
    {
        g_engfuncs->pfnCvar_SetValue("gl_monolights", g_saved_monolights);
        g_engfuncs->pfnCvar_SetValue("gl_overbright", g_saved_overbright);
        g_engfuncs->pfnCvar_SetValue("gl_fog", g_saved_fog);
        g_engfuncs->pfnCvar_SetValue("gl_lightholes", g_saved_lightholes);
        g_engfuncs->pfnCvar_SetValue("gl_flashblend", g_saved_flashblend);
        g_engfuncs->pfnCvar_SetValue("lightgamma", g_saved_lightgamma);
        g_engfuncs->pfnCvar_SetValue("texgamma", g_saved_texgamma);
        g_engfuncs->pfnCvar_SetValue("gamma", g_saved_gamma);
        g_engfuncs->pfnCvar_SetValue("brightness", g_saved_brightness);
    }

    g_nightmode_saved = -1;
}

void sh_grenade_path_draw(void)
{
    if (!g_cfg.misc.grenade_predict || !g_pmove)
        return;

    i32 wpn = 0;

    sh_player_t *local = sh_player_local();

    if (local)
        wpn = local->weapon_id;

    if (wpn != 4 && wpn != 9 && wpn != 25)
        return;

    vec3_t origin = g_pmove->origin;
    origin.z += g_pmove->view_ofs.z;

    vec3_t angles = g_pmove->angles;

    angles.x += g_pmove->punchangle.x;
    angles.y += g_pmove->punchangle.y;

    vec3_t vel = g_pmove->velocity;

    sh_grenade_path_t path;
    path.fuse_time = 0.0f;

    if (wpn == 4)
        path.fuse_time = 3.0f;
    else if (wpn == 9)
        path.fuse_time = 1.5f;
    else if (wpn == 25)
        path.fuse_time = 2.0f;

    sh_grenade_predict(&origin, &angles, &vel, &path);

    if (path.count < 2)
        return;

    for (i32 i = 0; i < path.count - 1; ++i)
    {
        vec2_t a, b;

        if (!sh_world_to_screen(&path.points[i], &a))
            continue;
        if (!sh_world_to_screen(&path.points[i + 1], &b))
            continue;

        f32 f = (f32)i / (f32)path.count;
        col4_t c = { 190, 235, 60, (u8)(230.0f * (1.0f - f * 0.6f)) };

        sh_draw_line(a.x, a.y, b.x, b.y, 1.5f, c);
    }

    if (path.hit)
    {
        vec2_t sp;

        if (sh_world_to_screen(&path.impact, &sp))
        {
            f32 r = 6.0f * sh_scale();

            col4_t c = { 255, 90, 90, 220 };
            sh_draw_line(sp.x - r, sp.y, sp.x + r, sp.y, 2.0f, c);
            sh_draw_line(sp.x, sp.y - r, sp.x, sp.y + r, 2.0f, c);

            if (g_cfg.misc.grenade_radius)
            {
                f32 rad = 0.0f;
                f32 fuse = 0.0f;

                switch (wpn)
                {
                case 9:
                    rad = 350.0f;
                    fuse = 3.0f;
                    break;
                case 11:
                    rad = 240.0f;
                    fuse = 1.5f;
                    break;
                case 13:
                    rad = 185.0f;
                    fuse = 2.0f;
                    break;
                case 25:
                    rad = 185.0f;
                    fuse = 2.0f;
                    break;
                default:
                    break;
                }

                if (rad > 0.0f)
                    sh_text_center(sp.x, sp.y - 12.0f,
                                   (col4_t)SH_COL_RGBA(255, 160, 80, 230),
                                   "%.1fm  r%.0f  %.1fs", rad / 39.37f, rad, fuse);
            }
        }
    }
}
