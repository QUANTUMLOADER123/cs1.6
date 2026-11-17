#include "../../include/sh_features.h"
#include "../../include/sh_math.h"
#include "../../include/sh_render.h"
#include "../../include/sh_bones.h"

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

    col4_t hp = { (u8)(255.0f * (1.0f - frac)), (u8)(255.0f * frac), 40, 255 };
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
    sh_draw_rect(x, b->top + h * (1.0f - frac), bar_w, h * frac,
                 (col4_t)SH_COL_RGBA(80, 160, 255, 255));
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

    const sh_bone_snapshot_t *snap = sh_bones_get(p->index);

    if (snap)
    {
        static const i32 links[][2] = {
            { sh_bone_head, sh_bone_neck },
            { sh_bone_neck, sh_bone_spine1 },
            { sh_bone_spine1, sh_bone_spine },
            { sh_bone_spine, sh_bone_pelvis },
            { sh_bone_spine1, sh_bone_l_upperarm },
            { sh_bone_l_upperarm, sh_bone_l_forearm },
            { sh_bone_l_forearm, sh_bone_l_hand },
            { sh_bone_spine1, sh_bone_r_upperarm },
            { sh_bone_r_upperarm, sh_bone_r_forearm },
            { sh_bone_r_forearm, sh_bone_r_hand },
            { sh_bone_pelvis, sh_bone_l_thigh },
            { sh_bone_l_thigh, sh_bone_l_calf },
            { sh_bone_l_calf, sh_bone_l_foot },
            { sh_bone_pelvis, sh_bone_r_thigh },
            { sh_bone_r_thigh, sh_bone_r_calf },
            { sh_bone_r_calf, sh_bone_r_foot }
        };

        for (u32 i = 0; i < sh_countof(links); ++i)
        {
            vec2_t a, bb;
            if (!sh_bones_world_to_screen(snap, links[i][0], &a))
                continue;
            if (!sh_bones_world_to_screen(snap, links[i][1], &bb))
                continue;
            sh_draw_line(a.x, a.y, bb.x, bb.y, 1.0f, c);
        }
        return;
    }

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

    if (p->health <= 0)
        return;

    if (p->distance > g_cfg.visuals.max_distance)
        return;

    if (g_cfg.visuals.visible_only && !p->visible)
        return;

    sh_box_t box;
    if (!sh_make_box(p, &box) || !box.valid)
        return;

    if (g_cfg.visuals.box)
        sh_draw_box(p, &box);

    if (g_cfg.visuals.health_bar)
        sh_draw_health(p, &box);

    sh_draw_armor(p, &box);
    sh_draw_name(p, &box);
    sh_draw_weapon(p, &box);
    sh_draw_distance(p, &box);
    sh_draw_head_dot(p, &box);
    sh_draw_skeleton(p, &box);
    sh_draw_snapline(p, &box);
}

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
}
