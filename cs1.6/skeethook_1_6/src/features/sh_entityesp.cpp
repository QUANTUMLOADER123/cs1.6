#include "../../include/sh_config.h"
#include "../../include/sh_features.h"
#include "../../include/sh_render.h"
#include "../../include/sh_sdk.h"
#include "../../include/sh_math.h"

#define sh_max_world_ents 96

typedef struct sh_went_s
{
    vec3_t origin;
    char name[32];
    f32 born;
    i32 kind;
    qbool active;
} sh_went_t;

static sh_went_t g_wents[sh_max_world_ents];
static i32 g_went_cursor;

static char sh_lower(char c)
{
    if (c >= 'A' && c <= 'Z')
        return (char)(c + 32);

    return c;
}

static i32 sh_has(const char *hay, const char *needle)
{
    if (!hay || !needle)
        return 0;

    for (i32 i = 0; hay[i]; ++i)
    {
        i32 k = 0;

        while (needle[k] && sh_lower(hay[i + k]) == sh_lower(needle[k]))
            ++k;

        if (!needle[k])
            return 1;
    }

    return 0;
}

static void sh_copy_name(char *dst, const char *src, i32 max)
{
    i32 i = 0;

    if (!dst || !src || max <= 0)
        return;

    const char *base = src;
    const char *p = src;

    while (*p)
    {
        if (*p == '/' || *p == '\\')
            base = p + 1;

        ++p;
    }

    while (base[i] && i < max - 1)
    {
        dst[i] = base[i];
        ++i;
    }

    dst[i] = 0;
}

void sh_entityesp_add(sh_cl_entity_t *ent, const char *model_name)
{
    if (!ent || !model_name || !g_cfg.visuals.entity_esp || !g_pmove)
        return;

    i32 kind = 0;

    if (sh_has(model_name, "w_c4"))
        kind = 1;
    else if (sh_has(model_name, "w_") && sh_has(model_name, ".mdl"))
        kind = 2;
    else
        return;

    sh_went_t *w = &g_wents[g_went_cursor];

    w->origin = ent->origin;
    w->born = g_pmove->time;
    w->kind = kind;
    w->active = 1;

    sh_copy_name(w->name, model_name, (i32)sizeof(w->name));

    g_went_cursor = (g_went_cursor + 1) % sh_max_world_ents;
}

void sh_entityesp_draw(void)
{
    if (!g_cfg.visuals.entity_esp)
        return;

    if (!g_pmove)
    {
        for (i32 i = 0; i < sh_max_world_ents; ++i)
            g_wents[i].active = 0;

        return;
    }

    f32 now = g_pmove->time;

    sh_text_esp_begin();

    for (i32 i = 0; i < sh_max_world_ents; ++i)
    {
        sh_went_t *w = &g_wents[i];

        if (!w->active)
            continue;

        if (now - w->born > 1.5f)
        {
            w->active = 0;
            continue;
        }

        vec3_t pos = w->origin;
        pos.z += 8.0f;

        vec2_t sp;

        if (!sh_world_to_screen(&pos, &sp))
            continue;

        col4_t c = (w->kind == 1)
                       ? (col4_t)SH_COL_RGBA(255, 80, 80, 240)
                       : (col4_t)SH_COL_RGBA(255, 210, 120, 220);

        sh_text_center(sp.x, sp.y, c, "%s", w->name);
    }

    sh_text_esp_end();
}
