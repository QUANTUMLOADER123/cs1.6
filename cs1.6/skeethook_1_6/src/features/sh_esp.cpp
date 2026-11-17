#include "../../include/sh_features.h"
#include "../../include/sh_math.h"
#include "../../include/sh_render.h"
#include "../../include/sh_ballistics.h"

#include <math.h>

typedef struct sh_box_s
{
    f32 left;
    f32 top;
    f32 right;
    f32 bottom;
    qbool valid;
} sh_box_t;

static qbool sh_make_box(const sh_player_t *p, sh_box_t *box)
{
    vec3_t top = p->origin;
    top.z += (p->ent->curstate.eflags & sh_fl_ducking) ? 54.0f : 72.0f;

    vec3_t bottom = p->origin;
    bottom.z -= 12.0f;

    vec2_t s_top, s_bottom;
    if (!sh_world_to_screen(&top, &s_top))
        return false;
    if (!sh_world_to_screen(&bottom, &s_bottom))
        return false;

    f32 h = s_bottom.y - s_top.y;
    if (h < 2.0f || h > sh_screen_height())
        return false;

    f32 w = h * 0.42f;

    box->left = s_top.x - w * 0.5f;
    box->right = s_top.x + w * 0.5f;
    box->top = s_top.y;
    box->bottom = s_bottom.y;
    box->valid = true;
    return true;
}

static void sh_draw_box(const sh_player_t *p, const sh_box_t *b)
{
    col4_t c = p->visible ? g_cfg.visuals.col_visible : g_cfg.visuals.col_hidden;

    if (g_cfg.visuals.box_outline)
    {
        sh_draw_rect(b->left - 1.0f, b->top - 1.0f,
                     b->right - b->left + 2.0f, b->bottom - b->top + 2.0f,
                     (col4_t)SH_COL_RGBA(0, 0, 0, 200));
    }

    if (g_cfg.visuals.box_style == 0)
    {
        sh_draw_rect_outline(b->left, b->top, b->right - b->left,
                             b->bottom - b->top, 1.0f, c);
    }
    else
    {
        f32 w = b->right - b->left;
        f32 h = b->bottom - b->top;
        f32 corner = w * 0.25f;

        sh_draw_rect(b->left, b->top, corner, 1.0f, c);
        sh_draw_rect(b->left, b->top, 1.0f, corner, c);
        sh_draw_rect(b->right - corner, b->top, corner, 1.0f, c);
        sh_draw_rect(b->right - 1.0f, b->top, 1.0f, corner, c);
        sh_draw_rect(b->left, b->bottom - 1.0f, corner, 1.0f, c);
        sh_draw_rect(b->left, b->bottom - corner, 1.0f, corner, c);
        sh_draw_rect(b->right - corner, b->bottom - 1.0f, corner, 1.0f, c);
        sh_draw_rect(b->right - 1.0f, b->bottom - corner, 1.0f, corner, c);
    }
}

static void sh_draw_health(const sh_player_t *p, const sh_box_t *b)
{
    f32 h = b->bottom - b->top;
    f32 frac = sh_clampf((f32)p->health / 100.0f, 0.0f, 1.0f);
    f32 bar_w = 3.0f;
    f32 x = b->left - bar_w - 3.0f;

    sh_draw_rect(x - 1.0f, b->top - 1.0f, bar_w + 2.0f, h + 2.0f,
                 (col4_t)SH_COL_RGBA(0, 0, 0, 180));

    col4_t hp = g_cfg.visuals.col_health;

    if (hp.a == 0)
        hp = (col4_t)SH_COL_RGBA(190, 235, 60, 255);

    sh_draw_rect(x, b->top + h * (1.0f - frac), bar_w, h * frac, hp);

    if (g_cfg.visuals.health)
    {
        char buf[16];
        i32 n = p->health;
        i32 i = 0;
        if (n >= 100)
            buf[i++] = '0' + (n / 100) % 10;
        if (n >= 10)
            buf[i++] = '0' + (n / 10) % 10;
        buf[i++] = '0' + n % 10;
        buf[i] = 0;

        sh_text(x - 16.0f, b->top + h * 0.5f - 5.0f, hp, "%s", buf);
    }
}

static void sh_draw_armor(const sh_player_t *p, const sh_box_t *b)
{
    if (!g_cfg.visuals.armor || p->armor <= 0)
        return;

    f32 h = b->bottom - b->top;
    f32 frac = sh_clampf((f32)p->armor / 100.0f, 0.0f, 1.0f);
    f32 bar_w = 3.0f;
    f32 x = b->right + 3.0f;

    sh_draw_rect(x, b->top, bar_w, h, (col4_t)SH_COL_RGBA(0, 0, 0, 150));
    col4_t ac = g_cfg.visuals.col_armor;

    if (ac.a == 0)
        ac = (col4_t)SH_COL_RGBA(80, 160, 255, 255);

    sh_draw_rect(x, b->top + h * (1.0f - frac), bar_w, h * frac, ac);
}

static void sh_draw_name(const sh_player_t *p, const sh_box_t *b)
{
    if (!g_cfg.visuals.name)
        return;

    sh_text_center((b->left + b->right) * 0.5f, b->top - sh_text_height() - 3.0f,
                   g_cfg.visuals.col_name, "%s", p->name[0] ? p->name : "player");
}

static void sh_draw_weapon(const sh_player_t *p, const sh_box_t *b)
{
    if (g_cfg.visuals.weapon_icon)
    {
        const char *icon = sh_weapon_icon(p->weapon_id);

        if (icon && icon[0])
        {
            f32 tw = sh_text_width(icon);
            sh_text(b->right + 4.0f, b->top + 2.0f,
                    g_cfg.visuals.col_weapon, "%s", icon);
            (void)tw;
        }
    }

    if (!g_cfg.visuals.weapon)
        return;

    sh_text_center((b->left + b->right) * 0.5f, b->bottom + 3.0f,
                   g_cfg.visuals.col_weapon, "%s", p->weapon_name);
}

static void sh_draw_distance(const sh_player_t *p, const sh_box_t *b)
{
    if (!g_cfg.visuals.distance)
        return;

    i32 meters = (i32)(p->distance * 0.0254f);
    sh_text_center((b->left + b->right) * 0.5f,
                   b->bottom + sh_text_height() + 4.0f,
                   (col4_t)SH_COL_RGBA(200, 200, 200, 255), "%dm", meters);
}

static void sh_draw_head_dot(const sh_player_t *p, const sh_box_t *b)
{
    if (!g_cfg.visuals.head_dot)
        return;

    f32 cx = (b->left + b->right) * 0.5f;
    f32 cy = b->top + (b->bottom - b->top) * 0.09f;
    sh_draw_circle(cx, cy, 3.0f, 12,
                   p->visible ? g_cfg.visuals.col_visible : g_cfg.visuals.col_hidden);
}

static void sh_draw_snapline(const sh_player_t *p, const sh_box_t *b)
{
    if (!g_cfg.visuals.snaplines)
        return;

    f32 cx = (b->left + b->right) * 0.5f;
    sh_draw_line((f32)sh_screen_width() * 0.5f, (f32)sh_screen_height(),
                 cx, b->bottom, 1.0f,
                 p->visible ? g_cfg.visuals.col_visible : g_cfg.visuals.col_hidden);
}

static void sh_draw_skeleton(const sh_player_t *p, const sh_box_t *b)
{
    if (!g_cfg.visuals.skeleton)
        return;

    col4_t c = p->visible ? g_cfg.visuals.col_skeleton : g_cfg.visuals.col_hidden;

    static const i32 bones_fallback[][2] = {
        { 0, 1 }, { 1, 2 }, { 2, 3 }, { 3, 4 },
        { 1, 5 }, { 5, 6 }, { 6, 7 },
        { 1, 8 }, { 8, 9 }, { 9, 10 }
    };

    f32 h = b->bottom - b->top;
    f32 w = b->right - b->left;
    f32 cx = (b->left + b->right) * 0.5f;

    vec2_t pts[11];
    pts[0] = (vec2_t){ cx, b->top + h * 0.08f };
    pts[1] = (vec2_t){ cx, b->top + h * 0.22f };
    pts[2] = (vec2_t){ cx, b->top + h * 0.48f };
    pts[3] = (vec2_t){ cx, b->top + h * 0.68f };
    pts[4] = (vec2_t){ cx - w * 0.12f, b->bottom };
    pts[5] = (vec2_t){ cx - w * 0.38f, b->top + h * 0.30f };
    pts[6] = (vec2_t){ cx - w * 0.48f, b->top + h * 0.58f };
    pts[7] = (vec2_t){ cx - w * 0.50f, b->top + h * 0.80f };
    pts[8] = (vec2_t){ cx + w * 0.38f, b->top + h * 0.30f };
    pts[9] = (vec2_t){ cx + w * 0.48f, b->top + h * 0.58f };
    pts[10] = (vec2_t){ cx + w * 0.50f, b->top + h * 0.80f };

    for (u32 i = 0; i < sh_countof(bones_fallback); ++i)
    {
        sh_draw_line(pts[bones_fallback[i][0]].x, pts[bones_fallback[i][0]].y,
                     pts[bones_fallback[i][1]].x, pts[bones_fallback[i][1]].y, 1.0f, c);
    }
}

void sh_esp_draw_player(const sh_player_t *p)
{
    if (!p || !p->ent)
        return;

    sh_text_esp_begin();

    if (p->health <= 0)
        return;

    if (p->distance > g_cfg.visuals.max_distance)
        return;

    if (g_cfg.visuals.visible_only && !p->visible)
        return;

    if (g_cfg.visuals.team_check)
    {
        sh_player_t *local = sh_player_local();

        if (local && local->team && p->team == local->team && p->index != local->index)
            return;
    }

    if (p->index == g_local_player && !g_cfg.esp.show_local)
        return;

    sh_box_t box;
    if (!sh_make_box(p, &box) || !box.valid)
        return;

    if (g_cfg.visuals.box)
        sh_draw_box(p, &box);

    if (g_cfg.visuals.health_bar)
        sh_draw_health(p, &box);

    if (g_cfg.visuals.ammo)
    {
        const sh_weapon_list_t *w = sh_weapon_get(p->weapon_id);

        if (w)
        {
            sh_text(box.right + 4.0f, box.top + 14.0f,
                    g_cfg.visuals.col_weapon, "%d/%d",
                    w->clip, w->clip_max);
        }
    }

    sh_draw_armor(p, &box);
    sh_draw_name(p, &box);
    sh_draw_weapon(p, &box);
    sh_draw_distance(p, &box);
    sh_draw_head_dot(p, &box);
    sh_draw_skeleton(p, &box);
    sh_draw_snapline(p, &box);

    sh_text_esp_end();
}

i32 sh_enemy_fire_count(void);
void sh_enemy_fire_tick(f32 now);

void sh_enemy_fire_draw(void)
{
    if (!g_cfg.misc.enemy_fire_indicator)
        return;

    f32 now = g_pmove ? g_pmove->time : 0.0f;
    sh_enemy_fire_tick(now);

    f32 s = sh_scale();
    f32 cx = (f32)sh_screen_width() * 0.5f;
    f32 cy = (f32)sh_screen_height() * 0.5f;

    sh_player_t *local = sh_player_local();

    if (!local)
        return;

    i32 n = sh_enemy_fire_count();

    if (n <= 0)
        return;

    col4_t c = { 255, 140, 60, (u8)(120 + n * 20 > 255 ? 255 : 120 + n * 20) };
    sh_text(cx - 20.0f * s, cy + 40.0f * s, c, "fire %d", n);
}

void sh_radar_draw(void);
void sh_spectator_list_draw(void);

void sh_crosshair_draw(void);

void sh_esp_run(void)
{
    if (!g_cfg.visuals.enabled)
        return;

    i32 count = sh_players_count();
    for (i32 i = 0; i < count; ++i)
    {
        sh_player_t *p = sh_player_get(i);
        if (p)
            sh_esp_draw_player(p);
    }

    sh_radar_draw();
    sh_spectator_list_draw();
    sh_enemy_fire_draw();
    sh_crosshair_draw();
}

i32 sh_enemy_fire_count(void);
void sh_enemy_fire_tick(f32 now);


void sh_radar_draw(void)
{
    if (!g_cfg.misc.radar_hack)
        return;

    i32 w = sh_screen_width();
    i32 h = sh_screen_height();
    f32 s = sh_scale();

    f32 rsize = 110.0f * s;
    f32 cx = (f32)w - rsize - 10.0f * s;
    f32 cy = 10.0f * s;

    col4_t bg = { 10, 10, 10, 200 };
    col4_t bd = { 45, 45, 45, 255 };

    sh_draw_rect(cx, cy, rsize, rsize, bg);
    sh_draw_rect_outline(cx, cy, rsize, rsize, 1.0f, bd);

    sh_player_t *local = sh_player_local();

    if (!local)
        return;

    f32 half = rsize * 0.5f;
    f32 center_x = cx + half;
    f32 center_y = cy + half;

    f32 scale = half / 2000.0f;

    sh_draw_line(center_x - 3.0f, center_y, center_x + 3.0f, center_y, 1.0f, bd);
    sh_draw_line(center_x, center_y - 3.0f, center_x, center_y + 3.0f, 1.0f, bd);

    for (i32 i = 1; i <= 32; ++i)
    {
        sh_player_t *p = sh_player_get(i);

        if (!p || !p->ent || p->index == g_local_player)
            continue;

        if (local->team && p->team == local->team)
            continue;

        if (p->health <= 0)
            continue;

        f32 dx = p->origin.x - local->origin.x;
        f32 dy = p->origin.y - local->origin.y;

        f32 px = center_x + dx * scale;
        f32 py = center_y - dy * scale;

        if (px < cx + 1.0f || px > cx + rsize - 1.0f)
            continue;
        if (py < cy + 1.0f || py > cy + rsize - 1.0f)
            continue;

        col4_t c = p->visible ? (col4_t){ 190, 235, 60, 255 }
                              : (col4_t){ 255, 90, 90, 255 };

        sh_draw_rect(px - 1.5f, py - 1.5f, 3.0f, 3.0f, c);
    }
}

void sh_spectator_list_draw(void)
{
    if (!g_cfg.visuals.spectator_list)
        return;

    f32 s = sh_scale();
    f32 x = 8.0f * s;
    f32 y = (f32)sh_screen_height() - 90.0f * s;

    sh_text(x, y, (col4_t){ 200, 200, 200, 220 }, "spectators");

    i32 n = 0;

    for (i32 i = 1; i <= 32; ++i)
    {
        sh_player_t *p = sh_player_get(i);

        if (!p || !p->ent || p->index == g_local_player)
            continue;

        if (p->health > 0)
            continue;

        sh_text(x, y + (14.0f + (f32)n * 10.0f) * s,
                (col4_t){ 150, 150, 150, 200 }, "%s", p->name);

        if (++n >= 4)
            break;
    }
}

void sh_crosshair_draw(void)
{
    if (!g_cfg.misc.crosshair || !g_pmove)
        return;

    f32 s = sh_scale();
    f32 cx = (f32)sh_screen_width() * 0.5f;
    f32 cy = (f32)sh_screen_height() * 0.5f;

    f32 gap = 3.0f * s;
    f32 len = 5.0f * s;

    sh_player_t *local = sh_player_local();

    if (local)
    {
        const sh_recoil_t *rec = sh_recoil_for(local->weapon_id);

        if (rec)
            gap += rec->up_base * s;
    }

    col4_t c = { 190, 235, 60, 200 };

    sh_draw_line(cx - gap - len, cy, cx - gap, cy, 1.0f, c);
    sh_draw_line(cx + gap, cy, cx + gap + len, cy, 1.0f, c);
    sh_draw_line(cx, cy - gap - len, cx, cy - gap, 1.0f, c);
    sh_draw_line(cx, cy + gap, cx, cy + gap + len, 1.0f, c);

    sh_draw_rect(cx - 1.0f, cy - 1.0f, 2.0f, 2.0f, c);
}
