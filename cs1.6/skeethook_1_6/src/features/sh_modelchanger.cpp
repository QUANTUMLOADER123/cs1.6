#include "../../include/sh_config.h"
#include "../../include/sh_sdk.h"
#include "../../include/sh_features.h"

static const char *g_player_models[] =
{
    "gign", "gsg9", "sas", "urban", "guerilla", "leet", "arctic", "terror",
    "vip", "militia", "spetsnaz", "seal", "vip_f", "riot"
};

#define sh_player_model_count ((i32)(sizeof(g_player_models) / sizeof(g_player_models[0])))

static i32 g_applied_index = -1;
static sh_cl_entity_t *g_applied_for;
static sh_model_t *g_cached[sh_player_model_count];

const char *sh_model_changer_name(i32 index)
{
    if (index < 0 || index >= sh_player_model_count)
        return "?";

    return g_player_models[index];
}

i32 sh_model_changer_count(void)
{
    return sh_player_model_count;
}

static sh_model_t *sh_model_changer_load(i32 index)
{
    if (index < 0 || index >= sh_player_model_count)
        return 0;

    if (g_cached[index])
        return g_cached[index];

    if (!g_engfuncs || !g_engfuncs->cl_load_model)
        return 0;

    char path[64];
    const char *base = g_player_models[index];

    i32 n = 0;

    const char *prefix = "models/player/";
    while (prefix[n])
    {
        path[n] = prefix[n];
        ++n;
    }

    i32 k = 0;
    while (base[k] && n < 56)
    {
        path[n] = base[k];
        ++n;
        ++k;
    }

    const char *suffix = "/" ;
    i32 m = 0;
    while (suffix[m] && n < 60)
    {
        path[n] = suffix[m];
        ++n;
        ++m;
    }

    k = 0;
    while (base[k] && n < 60)
    {
        path[n] = base[k];
        ++n;
        ++k;
    }

    suffix = ".mdl";
    m = 0;
    while (suffix[m] && n < 63)
    {
        path[n] = suffix[m];
        ++n;
        ++m;
    }

    path[n] = 0;

    i32 cl_index = 0;
    sh_model_t *mdl = g_engfuncs->cl_load_model(path, &cl_index);

    if (mdl)
        g_cached[index] = mdl;

    return mdl;
}

void sh_model_changer_tick(void)
{
    if (!g_cfg.visuals.model_changer)
    {
        if (g_applied_index >= 0)
        {
            g_applied_index = -1;
            g_applied_for = 0;
        }

        return;
    }

    sh_player_t *local = sh_player_local();

    if (!local || !local->ent)
        return;

    i32 want = g_cfg.visuals.model_index;

    if (want == g_applied_index && local->ent == g_applied_for)
        return;

    sh_model_t *mdl = sh_model_changer_load(want);

    if (!mdl)
        return;

    local->ent->model = mdl;

    g_applied_index = want;
    g_applied_for = local->ent;
}

i32 sh_model_changer_applied(void)
{
    return g_applied_index;
}
