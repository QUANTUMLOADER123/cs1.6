#include "../../include/sh_skybox.h"
#include "../../include/sh_config.h"
#include "../../include/sh_sdk.h"

static const char *g_skyboxes[sh_skybox_count] = {
    "desert", "dusk", "morning", "night", "sky1", "space",
    "storm", "tornado", "vegas", "venice", "alien1", "alien2"
};

static i32 g_applied = -1;
static i32 g_saved_valid;
static char g_saved[64];

const char *sh_skybox_name(i32 index)
{
    if (index < 0 || index >= sh_skybox_count)
        return "?";

    return g_skyboxes[index];
}

static void sh_skybox_store_saved(void)
{
    if (g_saved_valid || !g_engfuncs || !g_engfuncs->pfnGetCvarPointer)
        return;

    void *cv = g_engfuncs->pfnGetCvarPointer("sv_skyname");

    if (!cv)
        return;

    const char *val = *(const char **)((u8 *)cv + 12);

    if (!val)
        return;

    i32 n = 0;

    while (val[n] && n < 63)
    {
        g_saved[n] = val[n];
        ++n;
    }

    g_saved[n] = 0;
    g_saved_valid = 1;
}

void sh_skybox_apply(i32 index)
{
    if (!g_engfuncs || !g_engfuncs->pfnCvar_Set || index < 0 ||
        index >= sh_skybox_count)
        return;

    if (index == g_applied)
        return;

    sh_skybox_store_saved();

    g_engfuncs->pfnCvar_Set("sv_skyname", g_skyboxes[index]);
    g_applied = index;
}

void sh_skybox_restore(void)
{
    if (g_applied < 0)
        return;

    if (g_saved_valid && g_engfuncs && g_engfuncs->pfnCvar_Set)
        g_engfuncs->pfnCvar_Set("sv_skyname", g_saved);

    g_applied = -1;
}

i32 sh_skybox_current(void)
{
    return g_applied;
}

void sh_skybox_tick(void)
{
    if (!g_cfg.visuals.skybox_change)
    {
        if (g_applied >= 0)
            sh_skybox_restore();
        return;
    }

    sh_skybox_apply(g_cfg.visuals.skybox_index);
}
