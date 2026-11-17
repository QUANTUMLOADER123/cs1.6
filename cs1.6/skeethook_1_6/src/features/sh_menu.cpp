#include "../../include/sh_config.h"
#include "../../include/sh_movement.h"
#include "../../include/sh_grenade.h"
#include "../../include/sh_render.h"
#include "../../include/sh_types.h"
#include "../../include/sh_sdk.h"
#include "../../include/sh_features.h"

#include <windows.h>
#include <stdio.h>

static i32 g_tab;
static i32 g_visible;
static i32 g_selected;

static const char *g_tabs[] = {
    "rage", "antiaim", "visuals", "misc", "movement", "settings"
};

static void sh_menu_toggle(void)
{
    static i32 prev;
    i32 now = (GetAsyncKeyState(VK_INSERT) & 0x8000) != 0;

    if (now && !prev)
    {
        g_visible = !g_visible;
        if (!g_visible)
            sh_config_save();
    }
    prev = now;
}

static POINT g_mouse;
static qbool g_click;

static void sh_menu_input_begin(void)
{
    GetCursorPos(&g_mouse);

    HWND wnd = WindowFromPoint(g_mouse);

    if (wnd)
        ScreenToClient(wnd, &g_mouse);

    g_click = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
}

static qbool sh_mouse_in(f32 x, f32 y, f32 w, f32 h)
{
    return (f32)g_mouse.x >= x && (f32)g_mouse.x <= x + w &&
           (f32)g_mouse.y >= y && (f32)g_mouse.y <= y + h;
}

static qbool sh_mouse_clicked(void)
{
    return g_click;
}

static void sh_menu_checkbox(f32 x, f32 y, const char *label, qbool *value)
{
    f32 s = sh_scale();
    f32 box = 9.0f * s;

    sh_draw_rect(x, y, box, box, (col4_t)SH_COL_RGBA(18, 18, 18, 255));
    sh_draw_rect_outline(x, y, box, box, 1.0f, (col4_t)SH_COL_RGBA(70, 70, 70, 255));

    if (*value)
        sh_draw_rect(x + 2.0f * s, y + 2.0f * s, box - 4.0f * s, box - 4.0f * s,
                     (col4_t)SH_COL_RGBA(190, 235, 60, 255));

    sh_text(x + box + 6.0f * s, y, (col4_t)SH_COL_RGBA(215, 215, 215, 255), "%s", label);

    f32 tw = sh_text_width(label);

    if (sh_mouse_in(x, y - 2.0f * s, box + 6.0f * s + tw, box + 2.0f * s) &&
        sh_mouse_clicked())
    {
        static const void *last_toggle;
        static u32 last_tick;

        u32 now = GetTickCount();

        if (last_toggle != (const void *)value || now - last_tick > 250)
        {
            *value = !*value;
            last_toggle = (const void *)value;
            last_tick = now;
        }
    }
}

static void sh_menu_slider(f32 x, f32 y, f32 w, const char *label, f32 *value,
                           f32 lo, f32 hi, const char *suffix)
{
    f32 s = sh_scale();
    f32 tracky = y + 13.0f * s;

    sh_text(x, y, (col4_t)SH_COL_RGBA(215, 215, 215, 255), "%s", label);

    char val[32];
    i32 iv = (i32)*value;
    snprintf(val, sizeof(val), "%d%s", iv, suffix ? suffix : "");
    f32 vw = sh_text_width(val);
    sh_text(x + w - vw, y, (col4_t)SH_COL_RGBA(190, 235, 60, 255), "%s", val);

    sh_draw_rect(x, tracky, w, 3.0f * s, (col4_t)SH_COL_RGBA(38, 38, 38, 255));

    f32 frac = (*value - lo) / (hi - lo);
    sh_draw_rect(x, tracky, w * frac, 3.0f * s, (col4_t)SH_COL_RGBA(190, 235, 60, 255));

    if (sh_mouse_in(x, tracky - 4.0f * s, w, 11.0f * s) && sh_mouse_clicked())
    {
        f32 t = ((f32)g_mouse.x - x) / (w > 1.0f ? w : 1.0f);

        if (t < 0.0f) t = 0.0f;
        if (t > 1.0f) t = 1.0f;

        *value = lo + t * (hi - lo);
    }
}

static void sh_menu_combo(f32 x, f32 y, f32 w, const char *label, i32 *value,
                         const char **items, i32 count)
{
    f32 s = sh_scale();
    f32 h = 16.0f * s;

    static i32 open_combo = -1;
    static const void *owner;

    col4_t bg = { 26, 26, 26, 255 };
    col4_t bd = { 44, 44, 44, 255 };
    col4_t tx = { 200, 200, 200, 255 };

    sh_draw_rect(x, y, w, h, bg);
    sh_draw_rect_outline(x, y, w, h, 1.0f, bd);

    sh_text(x + 5.0f * s, y + 3.0f * s, tx, "%s", label);
    sh_text(x + w - 10.0f * s, y + 3.0f * s, tx, "v");

    qbool hovered = sh_mouse_in(x, y, w, h);

    if (hovered && sh_mouse_clicked())
    {
        if (open_combo == *value && owner == (const void *)value)
        {
            open_combo = -1;
            owner = 0;
        }
        else
        {
            open_combo = *value;
            owner = (const void *)value;
        }
    }

    if (owner != (const void *)value)
        return;

    for (i32 i = 0; i < count; ++i)
    {
        f32 iy = y + h + (f32)i * h;

        sh_draw_rect(x, iy, w, h, bg);
        sh_draw_rect_outline(x, iy, w, h, 1.0f, bd);

        col4_t it = (i == *value) ? (col4_t){ 120, 180, 255, 255 } : tx;
        sh_text(x + 5.0f * s, iy + 3.0f * s, it, "%s", items[i]);

        if (sh_mouse_in(x, iy, w, h) && sh_mouse_clicked())
        {
            *value = i;
            open_combo = -1;
            owner = 0;
        }
    }
}

static void sh_menu_section(const char *title, f32 x, f32 y)
{
    f32 s = sh_scale();
    sh_text(x, y, (col4_t)SH_COL_RGBA(190, 235, 60, 255), "%s", title);
    sh_draw_rect(x, y + 12.0f * s, 200.0f * s, 1.0f, (col4_t)SH_COL_RGBA(45, 45, 45, 255));
}

static void sh_menu_draw_rage(f32 x, f32 y)
{
    f32 s = sh_scale();
    f32 row = 20.0f * s;
    f32 colw = 210.0f * s;

    sh_menu_section("aimbot", x, y);

    f32 cy = y + 20.0f * s;
    sh_menu_checkbox(x, cy, "enabled", &g_cfg.aimbot.enabled);
    sh_menu_checkbox(x + colw, cy, "silent", &g_cfg.aimbot.silent);
    cy += row;

    sh_menu_checkbox(x, cy, "auto fire", &g_cfg.aimbot.auto_fire);
    sh_menu_checkbox(x + colw, cy, "auto wall", &g_cfg.aimbot.auto_wall);
    cy += row;

    sh_menu_checkbox(x, cy, "visible check", &g_cfg.aimbot.visible_check);
    cy += row + 6.0f * s;

    sh_menu_slider(x, cy, colw, "fov", &g_cfg.aimbot.fov, 1.0f, 180.0f, "");
    cy += row + 6.0f * s;
    sh_menu_slider(x, cy, colw, "smooth", &g_cfg.aimbot.smooth, 1.0f, 20.0f, "");
    cy += row + 6.0f * s;
    sh_menu_slider(x, cy, colw, "min damage", &g_cfg.aimbot.min_damage, 1.0f, 100.0f, "");
}

static const char *g_pitch_items[] = {
    "none", "down", "up", "fake down", "fake up", "spin", "static"
};

static const char *g_yaw_items[] = {
    "none", "backwards", "static left", "static right", "left", "right",
    "spin", "static", "random", "jitter", "fake sideways"
};

static void sh_menu_draw_antiaim(f32 x, f32 y)
{
    f32 s = sh_scale();
    f32 row = 20.0f * s;
    f32 colw = 210.0f * s;

    sh_menu_section("antiaim", x, y);

    f32 cy = y + 20.0f * s;
    sh_menu_checkbox(x, cy, "enabled", &g_cfg.antiaim.enabled);
    cy += row;

    sh_menu_combo(x, cy, colw, "pitch", &g_cfg.antiaim.pitch_mode,
                  g_pitch_items, 7);
    cy += row;

    sh_menu_combo(x, cy, colw, "yaw", &g_cfg.antiaim.yaw_mode,
                  g_yaw_items, 11);
    cy += row;

    sh_menu_slider(x, cy, colw, "roll", &g_cfg.antiaim.roll, -50.0f, 50.0f, "%.0f");
    cy += row + 8.0f * s;

    sh_menu_section("fake lag", x, cy);
    cy += 20.0f * s;

    sh_menu_checkbox(x, cy, "enabled", &g_cfg.antiaim.fake_lag);
    cy += row;

    static f32 g_fl_dbg;
    g_fl_dbg = (f32)g_cfg.antiaim.fake_lag_ticks;

    sh_menu_slider(x, cy, colw, "ticks", &g_fl_dbg, 0.0f, 22.0f, "%.0f");
    cy += row;

    if (g_fl_dbg >= 0.0f)
        g_cfg.antiaim.fake_lag_ticks = (i32)(g_fl_dbg + 0.5f);

    static f32 g_spin_dbg;
    g_spin_dbg = g_cfg.antiaim.spin_speed;
    sh_menu_slider(x, cy, colw, "spin speed", &g_spin_dbg, 0.0f, 10.0f, "%.1f");
    g_cfg.antiaim.spin_speed = g_spin_dbg;
}

static void sh_menu_draw_visuals(f32 x, f32 y)
{
    f32 s = sh_scale();
    f32 row = 20.0f * s;
    f32 colw = 210.0f * s;

    sh_menu_section("esp", x, y);

    f32 cy = y + 20.0f * s;
    sh_menu_checkbox(x, cy, "enabled", &g_cfg.visuals.enabled);
    sh_menu_checkbox(x + colw, cy, "box", &g_cfg.visuals.box);
    cy += row;

    sh_menu_checkbox(x, cy, "name", &g_cfg.visuals.name);
    sh_menu_checkbox(x + colw, cy, "health", &g_cfg.visuals.health_bar);
    cy += row;

    sh_menu_checkbox(x, cy, "weapon", &g_cfg.visuals.weapon);
    sh_menu_checkbox(x + colw, cy, "weapon icon", &g_cfg.visuals.weapon_icon);
    cy += row;

    sh_menu_checkbox(x, cy, "armor", &g_cfg.visuals.armor);
    sh_menu_checkbox(x + colw, cy, "distance", &g_cfg.visuals.distance);
    cy += row;

    sh_menu_checkbox(x, cy, "skeleton", &g_cfg.visuals.skeleton);
    sh_menu_checkbox(x + colw, cy, "head dot", &g_cfg.visuals.head_dot);
    cy += row;

    sh_menu_checkbox(x, cy, "snaplines", &g_cfg.visuals.snaplines);
    sh_menu_checkbox(x + colw, cy, "visible only", &g_cfg.visuals.visible_only);
    cy += row + 6.0f * s;

    sh_menu_slider(x, cy, colw, "max distance", &g_cfg.visuals.max_distance,
                   200.0f, 8192.0f, "");
}

static void sh_menu_draw_misc(f32 x, f32 y)
{
    f32 s = sh_scale();
    f32 row = 20.0f * s;
    f32 colw = 210.0f * s;

    sh_menu_section("movement", x, y);

    f32 cy = y + 20.0f * s;
    sh_menu_checkbox(x, cy, "edge jump", &g_cfg.misc.edge_jump);
    sh_menu_checkbox(x + colw, cy, "fast ladder", &g_cfg.misc.fast_ladder);
    cy += row;

    sh_menu_checkbox(x, cy, "no fall damage", &g_cfg.misc.no_fall_damage);
    sh_menu_checkbox(x + colw, cy, "auto pistol", &g_cfg.misc.auto_pistol);
    cy += row + 8.0f * s;

    sh_menu_section("combat", x, cy);
    cy += 20.0f * s;

    sh_menu_checkbox(x, cy, "no recoil", &g_cfg.misc.no_recoil);
    sh_menu_checkbox(x + colw, cy, "no spread", &g_cfg.misc.no_spread);
    cy += row;

    sh_menu_checkbox(x, cy, "fast reload", &g_cfg.misc.fast_reload);
    sh_menu_checkbox(x + colw, cy, "knife bot", &g_cfg.misc.knife_bot);
    cy += row + 8.0f * s;

    sh_menu_section("visual", x, cy);
    cy += 20.0f * s;

    sh_menu_checkbox(x, cy, "no flash", &g_cfg.misc.no_flash);
    sh_menu_checkbox(x + colw, cy, "no smoke", &g_cfg.misc.no_smoke);
    cy += row;

    sh_menu_checkbox(x, cy, "no scope", &g_cfg.misc.no_scope);
    sh_menu_checkbox(x + colw, cy, "crosshair", &g_cfg.misc.crosshair);
    cy += row;

    sh_menu_checkbox(x, cy, "radar hack", &g_cfg.misc.radar_hack);
    sh_menu_checkbox(x + colw, cy, "spectator list", &g_cfg.visuals.spectator_list);
}

static f32 g_movement_shots_dbg;

static void sh_menu_draw_movement(f32 x, f32 y)
{
    f32 s = sh_scale();
    f32 row = 20.0f * s;
    f32 colw = 210.0f * s;

    sh_menu_section("strafe", x, y);

    f32 cy = y + 20.0f * s;
    sh_menu_checkbox(x, cy, "auto strafe", &g_cfg.movement.autostrafe);
    sh_menu_checkbox(x + colw, cy, "quick stop", &g_cfg.movement.quick_stop);
    cy += row;

    sh_menu_checkbox(x, cy, "bunny hop", &g_cfg.movement.bhop);
    sh_menu_checkbox(x + colw, cy, "auto jump", &g_cfg.movement.auto_jump);
    cy += row + 8.0f * s;

    sh_menu_section("grenade", x, cy);
    cy += 20.0f * s;

    sh_menu_checkbox(x, cy, "super toss", &g_cfg.movement.super_toss);
    cy += row;

    sh_menu_slider(x, cy, colw, "toss power", &g_cfg.movement.super_toss_power, 0.0f, 100.0f, "%.0f");
    cy += row + 8.0f * s;

    sh_menu_section("info", x, cy);
    cy += 20.0f * s;

    g_movement_shots_dbg = (f32)sh_shots_count();

    sh_menu_slider(x, cy, colw, "shots", &g_movement_shots_dbg, 0.0f, 40.0f, "%.0f");
    cy += row + 8.0f * s;

    sh_menu_section("grenade predict", x, cy);
    cy += 20.0f * s;

    static f32 g_gren_angle_dbg;
    static f32 g_gren_range_dbg;

    g_gren_angle_dbg = sh_grenade_optimal_angle();
    g_gren_range_dbg = sh_grenade_range(g_gren_angle_dbg, 64.0f);

    sh_menu_slider(x, cy, colw, "best angle", &g_gren_angle_dbg, 0.0f, 90.0f, "%.0f");
    cy += row;

    sh_menu_slider(x, cy, colw, "range", &g_gren_range_dbg, 0.0f, 1200.0f, "%.0f");
}

static void sh_menu_draw_settings(f32 x, f32 y)
{
    f32 s = sh_scale();

    sh_menu_section("config", x, y);

    f32 cy = y + 22.0f * s;
    sh_text(x, cy, (col4_t)SH_COL_RGBA(200, 200, 200, 255), "insert - open menu");
    cy += 16.0f * s;
    sh_text(x, cy, (col4_t)SH_COL_RGBA(200, 200, 200, 255), "config autosaves on close");

    cy += 26.0f * s;
    qbool ready = sh_sdk_ready();
    sh_text(x, cy, ready ? (col4_t)SH_COL_RGBA(120, 255, 120, 255)
                         : (col4_t)SH_COL_RGBA(255, 90, 90, 255),
            "sdk: %s", ready ? "attached" : "waiting");
}

static void sh_draw_tab_button(f32 x, f32 y, f32 w, const char *label, i32 index)
{
    f32 s = sh_scale();
    qbool active = (g_tab == index);

    if (active)
    {
        sh_draw_rect(x, y, 2.0f, 13.0f * s, (col4_t)SH_COL_RGBA(190, 235, 60, 255));
        sh_text(x + 8.0f * s, y, (col4_t)SH_COL_RGBA(240, 240, 240, 255), "%s", label);
    }
    else
    {
        sh_text(x + 8.0f * s, y, (col4_t)SH_COL_RGBA(130, 130, 130, 255), "%s", label);
    }
}

void sh_menu_draw(void)
{
    sh_menu_toggle();
    if (!g_visible)
        return;

    f32 s = sh_scale();
    f32 w = 560.0f * s;
    f32 h = 400.0f * s;
    f32 ox = ((f32)sh_screen_width() - w) * 0.5f;
    f32 oy = ((f32)sh_screen_height() - h) * 0.5f;

    sh_draw_rect(ox, oy, w, h, (col4_t)SH_COL_RGBA(12, 12, 12, 245));
    sh_draw_rect_outline(ox, oy, w, h, 1.0f, (col4_t)SH_COL_RGBA(45, 45, 45, 255));

    sh_draw_rect(ox, oy, w, 26.0f * s, (col4_t)SH_COL_RGBA(20, 20, 20, 255));
    sh_text(ox + 12.0f * s, oy + 8.0f * s, (col4_t)SH_COL_RGBA(240, 240, 240, 255),
            "%s  v%s", SH_NAME, SH_VERSION);

    f32 sidebar = 92.0f * s;
    sh_draw_rect(ox, oy + 26.0f * s, sidebar, h - 26.0f * s,
                 (col4_t)SH_COL_RGBA(16, 16, 16, 255));

    sh_menu_input_begin();

    POINT mp = g_mouse;
    qbool clicked = g_click;

    for (i32 i = 0; i < (i32)sh_countof(g_tabs); ++i)
    {
        f32 ty = oy + 38.0f * s + (f32)i * 22.0f * s;
        sh_draw_tab_button(ox + 10.0f * s, ty, sidebar, g_tabs[i], i);

        if (clicked && mp.x >= ox && mp.x <= ox + sidebar &&
            mp.y >= ty && mp.y <= ty + 16.0f * s)
        {
            g_tab = i;
        }
    }

    f32 cx = ox + sidebar + 16.0f * s;
    f32 cy = oy + 40.0f * s;

    switch (g_tab)
    {
    case 0: sh_menu_draw_rage(cx, cy); break;
    case 1: sh_menu_draw_antiaim(cx, cy); break;
    case 2: sh_menu_draw_visuals(cx, cy); break;
    case 3: sh_menu_draw_misc(cx, cy); break;
    case 4: sh_menu_draw_movement(cx, cy); break;
    default: sh_menu_draw_settings(cx, cy); break;
    }
}
