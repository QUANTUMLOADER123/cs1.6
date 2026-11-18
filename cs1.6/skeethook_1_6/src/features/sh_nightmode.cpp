#include <windows.h>
#include "../ext/minhook/include/MinHook.h"
#include "../../include/sh_config.h"
#include "../../include/sh_scanner.h"
#include "../../include/sh_types.h"

typedef i32(__cdecl *build_lightmap_fn)(i32 surface, u8 *lightmap, i32 stride);
typedef void(__cdecl *rebuild_lightmaps_fn)(void);

static const u8 g_pat_build[] = {
    0x55, 0x8B, 0xEC, 0x83, 0xEC, 0x1C, 0xD9, 0x05, 0x00, 0x00, 0x00, 0x00,
    0xD8, 0x1D, 0x00, 0x00, 0x00, 0x00, 0xDF, 0xE0, 0xF6, 0xC4, 0x44,
};

static const u8 g_pat_format[] = {
    0xA1, 0x00, 0x00, 0x00, 0x00, 0x2D, 0x08, 0x19, 0x00, 0x00,
};

static const u8 g_pat_rebuild[] = {
    0x55, 0x8B, 0xEC, 0x51, 0x53, 0x56, 0x57, 0x68, 0x00, 0x80, 0x00, 0x00,
    0x6A, 0x00, 0x68, 0x00, 0x00, 0x00, 0x00, 0xE8, 0x00, 0x00, 0x00, 0x00,
    0xA1, 0x00, 0x00, 0x00, 0x00, 0x83, 0xC4, 0x0C, 0xBF, 0x01, 0x00, 0x00,
    0x00,
};

static const sh_pattern_t g_sig_build = {
    "build_lightmap", g_pat_build,
    "xxxxxxxx????xx????xxxxx",
    sizeof(g_pat_build)
};

static const sh_pattern_t g_sig_format = {
    "lightmap_format", g_pat_format,
    "x????xxxxx",
    sizeof(g_pat_format)
};

static const sh_pattern_t g_sig_rebuild = {
    "rebuild_lightmaps", g_pat_rebuild,
    "xxxxxxxxxxxxxxx????x????x????xxxxxxxx",
    sizeof(g_pat_rebuild)
};

static build_lightmap_fn g_build_orig;
static rebuild_lightmaps_fn g_rebuild;
static i32 *g_lightmap_format;
static qbool g_ready;
static f32 g_applied_r = -1.0f;
static f32 g_applied_g = -1.0f;
static f32 g_applied_b = -1.0f;
static qbool g_applied_on = (qbool)0;

static i32 __cdecl sh_build_lightmap_hook(i32 surface, u8 *lightmap, i32 stride)
{
    i32 result = 0;

    if (g_build_orig)
        result = g_build_orig(surface, lightmap, stride);
    else
        return 0;

    if (!g_cfg.visuals.lightmap_mode || !lightmap || !g_lightmap_format)
        return result;

    if (*g_lightmap_format != 6408)
        return result;

    const i16 *hdr = (const i16 *)((const u8 *)surface + 24);

    i32 width = (*hdr >> 4) + 1;
    i32 height = (*(hdr + 1) >> 4) + 1;

    if (width <= 0 || width > 256 || height <= 0 || height > 256)
        return result;

    f32 mr = g_cfg.visuals.lightmap_r;
    f32 mg = g_cfg.visuals.lightmap_g;
    f32 mb = g_cfg.visuals.lightmap_b;

    if (mr < 0.0f) mr = 0.0f;
    if (mr > 4.0f) mr = 4.0f;
    if (mg < 0.0f) mg = 0.0f;
    if (mg > 4.0f) mg = 4.0f;
    if (mb < 0.0f) mb = 0.0f;
    if (mb > 4.0f) mb = 4.0f;

    u8 *row = lightmap;

    for (i32 y = 0; y < height; ++y)
    {
        u8 *px = row;

        for (i32 x = 0; x < width; ++x)
        {
            f32 r = (f32)px[0] * mr;
            f32 g = (f32)px[1] * mg;
            f32 b = (f32)px[2] * mb;

            px[0] = (u8)(r >= 255.0f ? 255 : (i32)r);
            px[1] = (u8)(g >= 255.0f ? 255 : (i32)g);
            px[2] = (u8)(b >= 255.0f ? 255 : (i32)b);

            px += 4;
        }

        row += stride;
    }

    return result;
}

void sh_nightmode_init(void)
{
    if (g_ready)
        return;

    u8 *b = (u8 *)sh_find_pattern_in("hw.dll", &g_sig_build);
    u8 *f = (u8 *)sh_find_pattern_in("hw.dll", &g_sig_format);
    u8 *r = (u8 *)sh_find_pattern_in("hw.dll", &g_sig_rebuild);

    if (!b || !f || !r)
        return;

    g_lightmap_format = *(i32 **)(f + 1);
    g_rebuild = (rebuild_lightmaps_fn)r;

    if (MH_CreateHook((void *)b, (void *)sh_build_lightmap_hook,
                      (void **)&g_build_orig) == MH_OK)
    {
        MH_EnableHook((void *)b);
        g_ready = (qbool)1;
    }
}

void sh_nightmode_poll(void)
{
    if (!g_ready || !g_rebuild)
        return;

    f32 r = g_cfg.visuals.lightmap_r;
    f32 g = g_cfg.visuals.lightmap_g;
    f32 b = g_cfg.visuals.lightmap_b;

    qbool on = g_cfg.visuals.lightmap_mode;

    if (on == g_applied_on && r == g_applied_r && g == g_applied_g && b == g_applied_b)
        return;

    g_applied_on = on;
    g_applied_r = r;
    g_applied_g = g;
    g_applied_b = b;

    (void)on;
    g_rebuild();
}

qbool sh_nightmode_ready(void)
{
    return g_ready;
}
