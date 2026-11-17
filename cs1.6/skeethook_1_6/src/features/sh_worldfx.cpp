#include "../../include/sh_worldfx.h"
#include "../../include/sh_config.h"
#include "../../include/sh_sdk.h"
#include "../../include/sh_types.h"

static qbool g_tex_applied;
static f32 g_saved_picmip;
static f32 g_saved_detail;
static f32 g_saved_lightmap;

static f32 sh_cvar_get(const char *name)
{
    if (!g_engfuncs || !g_engfuncs->pfnGetCvarPointer)
        return 0.0f;

    void *cv = g_engfuncs->pfnGetCvarPointer(name);

    if (!cv)
        return 0.0f;

    return *(f32 *)((u8 *)cv + 12);
}

static void sh_cvar_set(const char *name, f32 v)
{
    if (g_engfuncs && g_engfuncs->pfnCvar_SetValue)
        g_engfuncs->pfnCvar_SetValue(name, v);
}

void sh_worldfx_tick(void)
{
    if (!g_engfuncs || !g_engfuncs->pfnCvar_SetValue)
        return;

    if (g_cfg.visuals.flat_textures)
    {
        if (!g_tex_applied)
        {
            g_saved_picmip = sh_cvar_get("gl_picmip");
            g_saved_detail = sh_cvar_get("r_detailtextures");
            g_saved_lightmap = sh_cvar_get("r_lightmap");
            g_tex_applied = 1;
        }

        sh_cvar_set("gl_picmip", 2.0f);
        sh_cvar_set("r_detailtextures", 0.0f);
        sh_cvar_set("r_lightmap", 1.0f);
    }
    else if (g_tex_applied)
    {
        sh_cvar_set("gl_picmip", g_saved_picmip);
        sh_cvar_set("r_detailtextures", g_saved_detail);
        sh_cvar_set("r_lightmap", g_saved_lightmap);
        g_tex_applied = 0;
    }
}

void sh_worldfx_hit_sound(void)
{
    sh_worldfx_hit_sound_damage(0);
}

void sh_worldfx_hit_sound_damage(i32 dmg)
{
    if (!g_cfg.visuals.hit_sound)
        return;

    if (!g_engfuncs || !g_engfuncs->pfnPlaySoundByName)
        return;

    f32 vol = g_cfg.visuals.hit_sound_volume;

    if (vol <= 0.0f)
        return;

    const char *snd = "weapons/ric1.wav";

    if (dmg >= 60)
        snd = "weapons/ric3.wav";
    else if (dmg >= 25)
        snd = "weapons/ric2.wav";

    g_engfuncs->pfnPlaySoundByName(snd, vol);
}
