#include "../../include/sh_skybox.h"
#include "../../include/sh_config.h"
#include "../../include/sh_sdk.h"

#include <windows.h>

static const char *g_skyboxes[sh_skybox_count] = {
    "desert", "dusk", "morning", "night", "sky1", "space",
    "storm", "tornado", "vegas", "venice", "alien1", "alien2"
};

static char g_custom[sh_skybox_max][32];
static i32 g_custom_count;

static i32 sh_sky_face_bit(const char *f)
{
    const char *names[6] = { "rt", "bk", "lf", "ft", "up", "dn" };

    for (i32 i = 0; i < 6; ++i)
    {
        i32 k = 0;

        while (names[i][k] && f[k] == names[i][k])
            ++k;

        if (!names[i][k])
            return 1 << i;
    }

    return 0;
}

static void sh_sky_add(const char *name, i32 len)
{
    for (i32 i = 0; i < g_custom_count; ++i)
    {
        i32 k = 0;

        while (k < len && g_custom[i][k] == name[k])
            ++k;

        if (k == len && g_custom[i][k] == 0)
            return;
    }

    if (g_custom_count >= sh_skybox_max || len <= 0 || len > 31)
        return;

    for (i32 i = 0; i < len; ++i)
        g_custom[g_custom_count][i] = name[i];

    g_custom[g_custom_count][len] = 0;
    ++g_custom_count;
}

void sh_skybox_scan(void)
{
    if (g_custom_count)
        return;

    const char *dirs[3] = { "gfx/env", "cstrike/gfx/env", "valve/gfx/env" };
    char query[128];
    WIN32_FIND_DATAA fd;

    for (i32 d = 0; d < 3; ++d)
    {
        i32 n = 0;

        while (dirs[d][n] && n < 100)
        {
            query[n] = dirs[d][n];
            ++n;
        }

        query[n++] = '\\';
        query[n++] = '*';
        query[n++] = '.';
        query[n++] = '*';
        query[n] = 0;

        HANDLE h = FindFirstFileA(query, &fd);

        if (h == INVALID_HANDLE_VALUE)
            continue;

        do
        {
            if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                continue;

            i32 dot = -1;

            for (i32 i = 0; fd.cFileName[i]; ++i)
            {
                if (fd.cFileName[i] == '.')
                    dot = i;
            }

            if (dot < 3)
                continue;

            if (!(fd.cFileName[dot + 1] == 't' && fd.cFileName[dot + 2] == 'g' && fd.cFileName[dot + 3] == 'a'))
                continue;

            char face[3];
            face[0] = fd.cFileName[dot - 2];
            face[1] = fd.cFileName[dot - 1];
            face[2] = 0;

            if (!sh_sky_face_bit(face))
                continue;

            sh_sky_add(fd.cFileName, dot - 2);
        } while (FindNextFileA(h, &fd));

        FindClose(h);

        if (g_custom_count)
            return;
    }
}

i32 sh_skybox_custom_count(void)
{
    return g_custom_count;
}

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
    if (!g_engfuncs || !g_engfuncs->pfnCvar_Set || index < 0)
        return;

    i32 limit = g_custom_count > 0 ? g_custom_count : sh_skybox_count;

    if (index >= limit)
        return;

    if (index == g_applied)
        return;

    sh_skybox_store_saved();

    g_engfuncs->pfnCvar_Set("sv_skyname", sh_skybox_name(index));
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
