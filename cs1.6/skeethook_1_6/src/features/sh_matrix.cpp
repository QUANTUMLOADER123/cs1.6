#include "../../include/sh_config.h"
#include "../../include/sh_scanner.h"
#include "../../include/sh_types.h"

static const u8 g_pat_world_transform[] = {
    0x55, 0x8B, 0xEC, 0x51, 0x8B, 0x45, 0x08, 0x8B, 0x4D, 0x0C, 0xD9, 0x05,
    0x00, 0x00, 0x00, 0x00, 0xD8, 0x08, 0xD9, 0x05, 0x00, 0x00, 0x00, 0x00,
    0xD8, 0x48, 0x08, 0xDE, 0xC1, 0xD9, 0x05, 0x00, 0x00, 0x00, 0x00,
};

static const sh_pattern_t g_sig_world_transform = {
    "world_transform", g_pat_world_transform,
    "xxxxxxxxxxxx????xxxx????xxxxxxx????",
    sizeof(g_pat_world_transform)
};

static const u8 g_pat_screen[] = {
    0x8B, 0x3D, 0x00, 0x00, 0x00, 0x00, 0xA1, 0x00, 0x00, 0x00, 0x00, 0x8B,
    0x15, 0x00, 0x00, 0x00, 0x00, 0x8B, 0xF3, 0x2B, 0x35, 0x00, 0x00, 0x00,
    0x00,
};

static const sh_pattern_t g_sig_screen = {
    "screen", g_pat_screen,
    "xx????x????xx????xxxx????",
    sizeof(g_pat_screen)
};

static const u8 g_pat_viewport[] = {
    0x8B, 0x0D, 0x00, 0x00, 0x00, 0x00, 0x83, 0xC4, 0x04, 0x52, 0x8B, 0x15,
    0x00, 0x00, 0x00, 0x00,
};

static const sh_pattern_t g_sig_viewport = {
    "viewport", g_pat_viewport,
    "xx????xxxxxx????",
    sizeof(g_pat_viewport)
};

static const f32 *g_world_matrix;
static const i32 *g_scr_ox;
static const i32 *g_scr_w;
static const i32 *g_scr_oy;
static const i32 *g_scr_h;
static const i32 *g_vp_x;
static const i32 *g_vp_y;

qbool sh_matrix_init(void)
{
    if (g_world_matrix)
        return true;

    u8 *wt = (u8 *)sh_find_pattern_in("hw.dll", &g_sig_world_transform);
    u8 *sc = (u8 *)sh_find_pattern_in("hw.dll", &g_sig_screen);
    u8 *vp = (u8 *)sh_find_pattern_in("hw.dll", &g_sig_viewport);

    if (!wt || !sc || !vp)
        return false;

    g_world_matrix = *(const f32 **)(wt + 0xC);
    g_scr_ox = *(const i32 **)(sc + 2);
    g_scr_w = *(const i32 **)(sc + 7);
    g_scr_oy = *(const i32 **)(sc + 0xD);
    g_scr_h = *(const i32 **)(sc + 0x15);
    g_vp_y = *(const i32 **)(vp + 2);
    g_vp_x = *(const i32 **)(vp + 0xC);

    if (!g_world_matrix || !g_scr_ox || !g_scr_w || !g_scr_oy || !g_scr_h ||
        !g_vp_x || !g_vp_y)
    {
        g_world_matrix = 0;
        return false;
    }

    return true;
}

qbool sh_matrix_ready(void)
{
    return g_world_matrix != 0;
}

qbool sh_matrix_world_to_screen(const vec3_t *world, vec2_t *out)
{
    if (!g_world_matrix || !world || !out)
        return false;

    const f32 *m = g_world_matrix;

    f32 cx = m[0] * world->x + m[4] * world->y + m[8] * world->z + m[12];
    f32 cy = m[1] * world->x + m[5] * world->y + m[9] * world->z + m[13];
    f32 cw = m[3] * world->x + m[7] * world->y + m[11] * world->z + m[15];

    if (cw <= 0.001f)
        return false;

    f32 iw = 1.0f / cw;

    out->x = (f32)(*g_vp_x + *g_scr_ox) + (1.0f + cx * iw) * 0.5f * (f32)(*g_scr_w);
    out->y = (f32)(*g_vp_y + *g_scr_oy) + (1.0f - cy * iw) * 0.5f * (f32)(*g_scr_h);

    return true;
}
