#include "../../include/sh_icons.h"
#include "../../include/sh_features.h"
#include "../../include/sh_render.h"

#include <string.h>
#include <stdio.h>

static sh_weapon_icon_t g_icons[sh_max_weapon_icons];
static qbool g_icons_ready;

static const char *g_icon_files[sh_max_weapon_icons] = {
    0,          "p228",      "glock18",   "scout",     "hegrenade",
    "xm1014",   "c4",        "mac10",     "aug",       "smokegrenade",
    "elite",    "fiveseven", "ump45",     "sg550",     "galil",
    "famas",    "usp",       "glock18",   "awp",       "mp5navy",
    "m249",     "m3",        "m4a1",      "tmp",       "g3sg1",
    "flashbang","deagle",    "sg552",     "ak47",      "knife",
    "p90"
};

static i32 sh_icon_find(const char *list, i32 count, const char *wanted,
                        sh_client_sprite_t *out)
{
    if (!list || count <= 0 || !wanted)
        return 0;

    for (i32 i = 0; i < count; ++i)
    {
        const sh_client_sprite_t *s = &((const sh_client_sprite_t *)list)[i];

        i32 j = 0;

        for (; j < 63; ++j)
        {
            char a = s->name[j];
            char b = wanted[j];

            if (a >= 'A' && a <= 'Z')
                a = (char)(a + 32);

            if (b >= 'A' && b <= 'Z')
                b = (char)(b + 32);

            if (a != b)
                break;

            if (!a)
            {
                if (out)
                    *out = *s;

                return 1;
            }
        }
    }

    return 0;
}

static qbool sh_icon_load_one(sh_weapon_icon_t *ic, const char *weapon)
{
    if (!ic || !weapon || !weapon[0])
        return 0;

    if (!g_engfuncs || !g_engfuncs->pfnSPR_GetList ||
        !g_engfuncs->pfnSPR_Load)
        return 0;

    char path[128];
    snprintf(path, sizeof(path), "sprites/%s.txt", weapon);

    i32 count = 0;
    sh_client_sprite_t *list = g_engfuncs->pfnSPR_GetList(path, &count);

    if (!list || count <= 0)
        return 0;

    sh_client_sprite_t found;
    memset(&found, 0, sizeof(found));

    if (!sh_icon_find((const char *)list, count, "weapon", &found))
        return 0;

    if (!found.sprite[0])
        return 0;

    char spr[128];
    snprintf(spr, sizeof(spr), "sprites/%s.spr", found.sprite);

    i32 h = g_engfuncs->pfnSPR_Load(spr);

    if (h <= 0)
        return 0;

    ic->spr = h;
    ic->frame = 0;
    ic->rect = found.rc;
    ic->loaded = 1;

    return 1;
}

void sh_icons_init(void)
{
    if (g_icons_ready)
        return;

    for (i32 i = 0; i < sh_max_weapon_icons; ++i)
    {
        g_icons[i].weapon_id = i;
        g_icons[i].loaded = 0;
        g_icons[i].spr = 0;
        g_icons[i].attempts = 0;
    }

    g_icons_ready = 1;
}

static sh_weapon_icon_t *sh_icon_get(i32 weapon_id)
{
    if (weapon_id <= 0 || weapon_id >= sh_max_weapon_icons)
        return 0;

    sh_weapon_icon_t *ic = &g_icons[weapon_id];

    if (ic->loaded)
        return ic;

    if (ic->attempts > 8)
        return 0;

    const char *file = g_icon_files[weapon_id];

    if (!file)
        return 0;

    ++ic->attempts;

    if (!sh_icon_load_one(ic, file))
        return 0;

    return ic;
}

const char *sh_icons_name(i32 weapon_id)
{
    if (weapon_id <= 0 || weapon_id >= sh_max_weapon_icons)
        return 0;

    return g_icon_files[weapon_id];
}

qbool sh_icons_draw(i32 weapon_id, f32 x, f32 y, f32 scale)
{
    if (!g_engfuncs || !g_engfuncs->pfnSPR_Set ||
        !g_engfuncs->pfnSPR_DrawAdditive)
        return 0;

    sh_weapon_icon_t *ic = sh_icon_get(weapon_id);

    if (!ic || !ic->loaded)
        return 0;

    f32 w = (f32)(ic->rect.right - ic->rect.left);
    f32 h = (f32)(ic->rect.bottom - ic->rect.top);

    if (w <= 0.0f || h <= 0.0f)
        return 0;

    g_engfuncs->pfnSPR_Set(ic->spr, 255, 255, 255);

    i32 px = (i32)(x < 0.0f ? x - 0.5f : x + 0.5f);
    i32 py = (i32)(y < 0.0f ? y - 0.5f : y + 0.5f);

    g_engfuncs->pfnSPR_DrawAdditive(ic->frame, px, py, &ic->rect);

    (void)scale;

    return 1;
}

f32 sh_icons_width(i32 weapon_id, f32 scale)
{
    if (weapon_id <= 0 || weapon_id >= sh_max_weapon_icons)
        return 0.0f;

    sh_weapon_icon_t *ic = &g_icons[weapon_id];

    if (!ic->loaded)
        return 0.0f;

    return (f32)(ic->rect.right - ic->rect.left) * scale;
}

f32 sh_icons_height(i32 weapon_id, f32 scale)
{
    if (weapon_id <= 0 || weapon_id >= sh_max_weapon_icons)
        return 0.0f;

    sh_weapon_icon_t *ic = &g_icons[weapon_id];

    if (!ic->loaded)
        return 0.0f;

    return (f32)(ic->rect.bottom - ic->rect.top) * scale;
}
