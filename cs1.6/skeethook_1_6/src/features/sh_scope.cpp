#include "../../include/sh_config.h"
#include "../../include/sh_render.h"
#include "../../include/sh_scanner.h"
#include "../../include/sh_sdk.h"
#include "../../include/sh_types.h"

static const u8 g_pat_scoped[] = { 0x83, 0x3D, 0x00, 0x00, 0x00, 0x00, 0x5A };

static const sh_pattern_t g_sig_scoped = {
    "scoped_fov", g_pat_scoped, "xx????x", sizeof(g_pat_scoped)
};

static i32 *g_scoped_fov;

void sh_scope_init(void)
{
    if (g_scoped_fov)
        return;

    u8 *hit = (u8 *)sh_find_pattern_in("client.dll", &g_sig_scoped);

    if (!hit)
        return;

    g_scoped_fov = *(i32 **)(hit + 2);
}

qbool sh_scope_active(void)
{
    if (!g_scoped_fov)
        return false;

    return *g_scoped_fov < 90;
}

void sh_scope_draw(void)
{
    if (!g_cfg.visuals.custom_scope || !sh_scope_active())
        return;

    f32 w = (f32)sh_screen_width();
    f32 h = (f32)sh_screen_height();
    f32 cx = w * 0.5f;
    f32 cy = h * 0.5f;

    f32 gap = g_cfg.visuals.scope_gap;
    f32 len = g_cfg.visuals.scope_len;
    f32 th = g_cfg.visuals.scope_thick;

    col4_t c = SH_COL_RGBA(0, 0, 0, 255);

    sh_draw_line(cx - gap - len, cy, cx - gap, cy, th, c);
    sh_draw_line(cx + gap, cy, cx + gap + len, cy, th, c);
    sh_draw_line(cx, cy - gap - len, cx, cy - gap, th, c);
    sh_draw_line(cx, cy + gap, cx, cy + gap + len, th, c);
}
