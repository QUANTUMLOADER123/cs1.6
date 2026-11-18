#include "../../include/sh_config.h"
#include "../../include/sh_math.h"
#include "../../include/sh_scanner.h"
#include "../../include/sh_types.h"

typedef void(__stdcall *sh_gl_frustum_fn)(f64 left, f64 right, f64 bottom,
                                          f64 top, f64 z_near, f64 z_far);

static const u8 g_frustum_pat[] = {
    0xDD, 0x45, 0x08, 0xDC, 0x0D, 0x00, 0x00, 0x00, 0x00, 0x83, 0xEC, 0x08,
    0xDC, 0x35, 0x00, 0x00, 0x00, 0x00, 0xDD, 0x1C, 0x24, 0xE8, 0x00, 0x00,
    0x00, 0x00, 0xDC, 0x4D, 0x18, 0x8B, 0x45, 0x24, 0x8B, 0x4D, 0x20, 0x8B,
    0x55, 0x1C, 0x83, 0xC4, 0x08, 0xDD, 0x5D, 0x08, 0xDD, 0x45, 0x08, 0xD9,
    0xE0, 0xDD, 0x5D, 0xF8, 0xDD, 0x45, 0xF8, 0xDC, 0x4D, 0x10, 0x50, 0x8B,
    0x45, 0x18, 0x51, 0x8B, 0x4D, 0x0C, 0x52, 0x8B, 0x55, 0x08, 0xDD, 0x5D,
    0xF0, 0xDD, 0x45, 0x08, 0xDC, 0x4D, 0x10, 0x50, 0x8B, 0x45, 0xFC, 0x51,
    0x8B, 0x4D, 0xF8, 0x52, 0x50, 0xDD, 0x5D, 0x10, 0x8B, 0x55, 0x14, 0x8B,
    0x45, 0x10, 0x51, 0x8B, 0x4D, 0xF4, 0x52, 0x8B, 0x55, 0xF0, 0x50, 0x51,
    0x52, 0xFF, 0x15, 0x00, 0x00, 0x00, 0x00,
};

static const sh_pattern_t g_frustum_sig = {
    "gl_frustum", g_frustum_pat,
    "xxxxx????xxxxx????xxxx????xxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
    "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx?"
    "???",
    sizeof(g_frustum_pat)
};

static sh_gl_frustum_fn g_frustum_orig;
static u32 *g_frustum_slot;
static qbool g_hooked;

static void __stdcall sh_gl_frustum_hook(f64 left, f64 right, f64 bottom,
                                        f64 top, f64 z_near, f64 z_far)
{
    if (!g_frustum_orig)
        return;

    if (g_cfg.visuals.aspect_ratio)
    {
        f64 horiz = right - left;
        f64 vert = top - bottom;

        if (horiz > 0.0 && vert > 0.0)
        {
            f32 a = g_cfg.visuals.aspect_value;

            if (a < 0.5f)
                a = 0.5f;
            if (a > 3.0f)
                a = 3.0f;

            f64 scale = (f64)a * vert / horiz;

            left *= scale;
            right *= scale;
        }
    }

    g_frustum_orig(left, right, bottom, top, z_near, z_far);
}

void sh_aspect_init(void)
{
    if (g_frustum_slot)
        return;

    u8 *hit = (u8 *)sh_find_pattern_in("hw.dll", &g_frustum_sig);

    if (!hit)
        return;

    g_frustum_slot = *(u32 **)(hit + 0x6F);
}

void sh_aspect_poll(void)
{
    if (!g_frustum_slot)
        return;

    u32 *slot = g_frustum_slot;
    void *stored = (void *)*slot;

    if (!stored || stored == (void *)sh_gl_frustum_hook)
        return;

    if (!g_frustum_orig)
        g_frustum_orig = (sh_gl_frustum_fn)stored;

    if (!g_cfg.visuals.aspect_ratio)
        return;

    *slot = (u32)sh_gl_frustum_hook;
    g_hooked = (qbool)1;
}

void sh_aspect_shutdown(void)
{
    if (!g_frustum_slot || !g_frustum_orig || !g_hooked)
        return;

    if ((void *)*g_frustum_slot == (void *)sh_gl_frustum_hook)
        *g_frustum_slot = (u32)g_frustum_orig;

    g_hooked = (qbool)0;
}

qbool sh_aspect_ready(void)
{
    return g_frustum_slot != 0;
}
