#include "../../include/sh_features.h"
#include "../../include/sh_autowall.h"
#include "../../include/sh_icons.h"
#include "../../include/sh_math.h"
#include "../../include/sh_render.h"
#include "../../include/sh_ballistics.h"
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
    top.z += (p->ent->curstate.usehull == 1) ? 54.0f : 72.0f;

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
        if (!sh_icons_draw(p->weapon_id, b->right + 4.0f, b->top + 2.0f,
                           1.0f))
        {
            const char *icon = sh_weapon_icon(p->weapon_id);

            if (icon && icon[0])
                sh_text(b->right + 4.0f, b->top + 2.0f,
                        g_cfg.visuals.col_weapon, "%s", icon);
        }
    }

    if (!g_cfg.visuals.weapon)
        return;

    if (sh_events_is_firing(p->index, 0.25f))
    {
        sh_text_center((b->left + b->right) * 0.5f, b->bottom + 3.0f,
                       (col4_t)SH_COL_RGBA(255, 80, 80, 255), "%s*",
                       p->weapon_name);
    }
    else
    {
        sh_text_center((b->left + b->right) * 0.5f, b->bottom + 3.0f,
                       g_cfg.visuals.col_weapon, "%s", p->weapon_name);
    }
}

static qbool sh_esp_trace(void *user, const vec3_t *start, const vec3_t *end, sh_trace_t *out)
{
    (void)user;

    if (!out)
        return 0;

    out->fraction = 1.0f;
    out->end = *end;
    out->normal = (vec3_t){ 0.0f, 0.0f, 1.0f };
    out->hitgroup = 0;
    out->char_texture = 0;
    out->entity = -1;

    f32 frac = 1.0f;

    if (sh_trace_fraction(start, end, &frac))
    {
        out->char_texture = sh_trace_material(start, end);
        out->fraction = frac;
        out->entity = 0;

        out->end.x = start->x + (end->x - start->x) * frac;
        out->end.y = start->y + (end->y - start->y) * frac;
        out->end.z = start->z + (end->z - start->z) * frac;

        out->normal.x = start->x - out->end.x;
        out->normal.y = start->y - out->end.y;
        out->normal.z = start->z - out->end.z;

        f32 len = sh_sqrt(out->normal.x * out->normal.x
                        + out->normal.y * out->normal.y
                        + out->normal.z * out->normal.z);

        if (len > 0.0f)
        {
            out->normal.x /= len;
            out->normal.y /= len;
            out->normal.z /= len;
        }
    }

    return 1;
}

static f32 sh_player_damage_to(const sh_player_t *p)
{
    sh_player_t *local = sh_player_local();

    if (!local || !g_pmove || !p || !p->ent)
        return 0.0f;

    const sh_weapon_ballistic_t *wb = sh_weapon_ballistic_for(local->weapon_id);

    if (!wb || wb->damage <= 0.0f)
        return 0.0f;

    vec3_t eye = g_pmove->origin;
    eye.z += g_pmove->view_ofs.z;

    vec3_t aim = p->origin;
    aim.z += 44.0f;

    sh_autowall_ctx_t aw;
    aw.start = eye;
    aw.end = aim;
    aw.bullet_id = 1;
    aw.penetration = wb->penetration;
    aw.base_damage = wb->damage;
    aw.range_modifier = wb->range_modifier;
    aw.penetrate_world = 1;
    aw.trace = sh_esp_trace;
    aw.user = 0;

    f32 dmg = sh_autowall_damage(&aw);

    i32 hg = 2;

    if (g_cfg.visuals.damage_hitbox)
    {
        switch (g_cfg.aimbot.hitbox)
        {
        case 1:
            hg = 1;
            break;
        case 2:
            hg = 2;
            break;
        case 3:
            hg = 3;
            break;
        default:
            hg = 2;
            break;
        }
    }

    dmg *= (f32)sh_hitgroup_multiplier(hg);

    return dmg;
}

static void sh_draw_damage(const sh_player_t *p, const sh_box_t *b)
{
    if (!g_cfg.visuals.damage_info)
        return;

    f32 dmg = sh_player_damage_to(p);

    if (dmg < 1.0f)
        return;

    col4_t c = (dmg >= 100.0f)
                   ? (col4_t)SH_COL_RGBA(255, 60, 60, 230)
                   : (col4_t)SH_COL_RGBA(255, 200, 90, 220);

    sh_text(b->right + 4.0f, b->top + sh_text_height() + 2.0f, c,
                 "-%d", (i32)dmg);
}

static void sh_draw_history(const sh_player_t *p)
{
    if (!g_cfg.visuals.draw_history)
        return;

    f32 alpha_step = 200.0f / (f32)sh_hist_max;
    f32 fade = 200.0f;

    for (i32 k = 1; k < sh_hist_max; ++k)
    {
        i32 a = (p->hist_head - k + sh_hist_max) % sh_hist_max;
        i32 b = (p->hist_head - k - 1 + sh_hist_max) % sh_hist_max;

        if (p->hist[a].time <= 0.0f || p->hist[b].time <= 0.0f)
            continue;

        vec3_t pa = p->hist[a].origin;
        vec3_t pb = p->hist[b].origin;

        pa.z += 36.0f;
        pb.z += 36.0f;

        vec2_t sa, sb;

        if (!sh_world_to_screen(&pa, &sa))
            continue;
        if (!sh_world_to_screen(&pb, &sb))
            continue;

        col4_t c = SH_COL_RGBA(190, 190, 190, (u8)fade);

        sh_draw_line(sa.x, sa.y, sb.x, sb.y, 1.0f, c);

        fade -= alpha_step;

        if (fade < 20.0f)
            break;
    }
}

static void sh_draw_hitboxes(const sh_player_t *p)
{
    if (!g_cfg.visuals.hitboxes)
        return;

    const sh_bone_snapshot_t *snap = sh_bones_get(p->index);

    if (!snap || !snap->valid)
        return;

    static const i32 edges[12][2] = {
        { 0, 1 }, { 1, 3 }, { 3, 2 }, { 2, 0 },
        { 4, 5 }, { 5, 7 }, { 7, 6 }, { 6, 4 },
        { 0, 4 }, { 1, 5 }, { 2, 6 }, { 3, 7 }
    };

    col4_t c = g_cfg.visuals.col_hitbox;

    for (i32 b = 0; b < snap->bbox_count; ++b)
    {
        vec3_t pts[8];

        if (!sh_bones_bbox_corners(snap, b, pts))
            continue;

        vec2_t sp[8];
        qbool ok[8];

        for (i32 k = 0; k < 8; ++k)
            ok[k] = sh_world_to_screen(&pts[k], &sp[k]);

        if (snap->bbox[b].bone == 0 && g_cfg.visuals.col_hitbox_head.a > 0)
            c = g_cfg.visuals.col_hitbox_head;

        for (i32 e = 0; e < 12; ++e)
        {
            i32 a0 = edges[e][0];
            i32 a1 = edges[e][1];

            if (!ok[a0] || !ok[a1])
                continue;

            sh_draw_line(sp[a0].x, sp[a0].y, sp[a1].x, sp[a1].y, 1.0f, c);
        }
    }
}

static void sh_draw_fake_marker(const sh_player_t *p, const sh_box_t *b)
{
    if (!g_cfg.visuals.draw_fakes)
        return;

    if (!p->fake_ent)
        return;

    sh_text_center((b->left + b->right) * 0.5f, b->top - sh_text_height() - 3.0f,
                   (col4_t)SH_COL_RGBA(255, 60, 60, 255), "FAKE");
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

    const sh_bone_snapshot_t *snap = sh_bones_get(p->index);

    if (snap)
    {
        static const i32 links[][2] = {
            { sh_bone_head, sh_bone_neck },
            { sh_bone_neck, sh_bone_spine3 },
            { sh_bone_spine3, sh_bone_spine2 },
            { sh_bone_spine2, sh_bone_spine1 },
            { sh_bone_spine1, sh_bone_pelvis },

            { sh_bone_spine3, sh_bone_l_clavicle },
            { sh_bone_l_clavicle, sh_bone_l_upperarm },
            { sh_bone_l_upperarm, sh_bone_l_forearm },
            { sh_bone_l_forearm, sh_bone_l_hand },
            { sh_bone_spine3, sh_bone_r_clavicle },
            { sh_bone_r_clavicle, sh_bone_r_upperarm },
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
    sh_draw_hitboxes(p);
    sh_draw_damage(p, &box);
    sh_draw_history(p);
    sh_draw_fake_marker(p, &box);
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

void sh_wallbang_draw(void)
{
    if (!g_cfg.visuals.penetration_info)
        return;

    sh_player_t *local = sh_player_local();

    if (!local || local->health <= 0)
        return;

    if (!g_pmove)
        return;

    vec3_t eye = g_pmove->origin;
    eye.z += g_pmove->view_ofs.z;

    vec3_t fwd;
    vec3_t ang = g_pmove->angles;

    vec3_t right, up;
    sh_angle_vectors(&ang, &fwd, &right, &up);

    i32 walls = sh_autowall_walls(&eye, &fwd, local->weapon_id);

    if (walls <= 0)
        return;

    f32 s = sh_scale();
    f32 cx = (f32)sh_screen_width() * 0.5f;
    f32 cy = (f32)sh_screen_height() * 0.5f;

    sh_text_center(cx, cy + 32.0f * s,
                   (col4_t)SH_COL_RGBA(0, 255, 0, 230), "wallbang (%d)",
                   walls);
}

void sh_esp_run(void)
{
    if (!g_cfg.visuals.enabled)
        return;

    for (i32 i = 1; i <= 32; ++i)
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
