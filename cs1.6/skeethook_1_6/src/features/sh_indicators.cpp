#include "../../include/sh_config.h"
#include "../../include/sh_render.h"
#include "../../include/sh_types.h"
#include "../../include/sh_movement.h"
#include "../../include/sh_fakelag.h"

typedef struct sh_indicator_s
{
    const char *label;
    qbool active;
} sh_indicator_t;

void sh_indicators_draw(void)
{
    static const f32 pad = 6.0f;

    sh_indicator_t list[24];
    i32 n = 0;

    if (g_cfg.aimbot.enabled) { list[n].label = "aimbot"; list[n++].active = 1; }
    if (g_cfg.aimbot.silent) { list[n].label = "silent"; list[n++].active = 1; }
    if (g_cfg.aimbot.double_tap) { list[n].label = "dt"; list[n++].active = 1; }
    if (g_cfg.aimbot.penetration) { list[n].label = "pen"; list[n++].active = 1; }
    if (g_cfg.aimbot.auto_stop) { list[n].label = "astop"; list[n++].active = 1; }
    if (g_cfg.antiaim.enabled) { list[n].label = "aa"; list[n++].active = 1; }
    if (g_cfg.antiaim.fake_lag) { list[n].label = "fl"; list[n++].active = 1; }
    if (g_cfg.movement.bhop) { list[n].label = "bhop"; list[n++].active = 1; }
    if (g_cfg.movement.autostrafe) { list[n].label = "strafe"; list[n++].active = 1; }
    if (g_cfg.movement.quick_stop) { list[n].label = "qstop"; list[n++].active = 1; }
    if (g_cfg.misc.no_recoil) { list[n].label = "norecoil"; list[n++].active = 1; }
    if (g_cfg.misc.knife_bot) { list[n].label = "knife"; list[n++].active = 1; }
    if (g_cfg.misc.auto_pistol) { list[n].label = "apistol"; list[n++].active = 1; }
    if (g_cfg.visuals.nightmode) { list[n].label = "night"; list[n++].active = 1; }
    if (g_cfg.visuals.tracer) { list[n].label = "tracer"; list[n++].active = 1; }
    if (g_cfg.visuals.chams) { list[n].label = "chams"; list[n++].active = 1; }

    if (n == 0)
        return;

    f32 s = sh_scale();
    f32 x = pad * s;
    f32 y = 40.0f * s;
    f32 lh = 11.0f * s;

    for (i32 i = 0; i < n; ++i)
    {
        col4_t c = { 190, 235, 60, 220 };
        sh_text(x, y, c, "%s", list[i].label);
        y += lh;
    }

    if (g_cfg.antiaim.fake_lag)
    {
        f32 fl_y = 40.0f * s + (f32)n * lh;
        sh_text(x, fl_y, (col4_t){ 255, 160, 60, 220 }, "choke %d", sh_fakelag_choking());
    }

    if (g_cfg.movement.bhop)
    {
        f32 st_y = 40.0f * s + (f32)n * lh + 11.0f * s;
        sh_text(x, st_y, (col4_t){ 120, 180, 255, 200 }, "shots %d", sh_shots_count());
    }
}
