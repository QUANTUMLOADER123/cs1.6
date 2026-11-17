#include "../../include/sh_rage.h"
#include "../../include/sh_config.h"
#include "../../include/sh_math.h"
#include "../../include/sh_features.h"
#include "../../include/sh_visuals.h"
#include "../../include/sh_hitmarker.h"
#include "../../include/sh_worldfx.h"
#include "../../include/sh_autowall.h"
#include "../../include/sh_bones.h"
#include "../../include/sh_nospread.h"
#include "../../include/sh_ballistics.h"

#include <windows.h>


static sh_rage_result_t g_rage;

static qbool sh_rage_trace(void *user, const vec3_t *start, const vec3_t *end, sh_trace_t *out)
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

        if (len > 0.001f)
        {
            out->normal.x /= len;
            out->normal.y /= len;
            out->normal.z /= len;
        }
        else
        {
            out->normal = (vec3_t){ 0.0f, 0.0f, 1.0f };
        }
    }

    return 1;
}
#define sh_autostop_threshold 1.0f

static f32 g_last_attack_sent;

extern f32 g_local_fov;
extern i32 g_local_weapon_id;


void sh_rage_scan(sh_rage_result_t *out)
{
    if (!out)
        return;

    out->found = 0;
    out->candidates = 0;
    out->target.damage = 0.0f;

    sh_player_t *local_p = sh_player_local();

    if (!local_p)
        return;

    vec3_t eye = local_p->origin;
    eye.z += 28.0f;

    vec3_t view = { 0.0f, 0.0f, 0.0f };

    if (g_engfuncs && g_engfuncs->pfnGetViewAngles)
        g_engfuncs->pfnGetViewAngles((f32 *)&view);

    vec3_t fwd, right, up;
    sh_angle_vectors(&view, &fwd, &right, &up);

    f32 best_fov = g_cfg.aimbot.fov;
    f32 best_score = 0.0f;
    f32 max_dist = g_cfg.visuals.max_distance;

    for (i32 i = 1; i <= 32; ++i)
    {
        const sh_player_t *p = sh_player_get(i);

        if (!p || !p->ent)
            continue;

        if (p->index == g_local_player)
            continue;

        if (g_cfg.visuals.team_check && local_p->team &&
            p->team == local_p->team)
            continue;

        if (g_cfg.visuals.visible_only && !p->visible)
            continue;

        if (p->health <= 0)
            continue;

        vec3_t center = p->origin;
        i32 hit_grp = 2;

        {
            const sh_bone_snapshot_t *snap = sh_bones_get(p->index);
            i32 want = -1;

            switch (g_cfg.aimbot.hitbox)
            {
            case 1:
                want = sh_bone_head;
                hit_grp = 1;
                break;
            case 2:
                want = sh_bone_spine3;
                hit_grp = 2;
                break;
            case 3:
                want = sh_bone_pelvis;
                hit_grp = 3;
                break;
            default:
                want = sh_bone_spine3;
                hit_grp = 2;
                break;
            }

            if (snap && snap->valid && want >= 0 && snap->pos[want].x != 0.0f)
            {
                center = snap->pos[want];
            }
            else
            {
                f32 h = 36.0f;

                if (g_cfg.aimbot.hitbox == 1)
                    h = 64.0f;
                else if (g_cfg.aimbot.hitbox == 2)
                    h = 44.0f;
                else if (g_cfg.aimbot.hitbox == 3)
                    h = 18.0f;

                if (p->ent && p->ent->curstate.usehull == 1)
                    h *= 0.72f;

                center.z += h;
            }
        }

        vec3_t d = center;

        d.x -= eye.x;
        d.y -= eye.y;
        d.z -= eye.z;

        f32 dist = sh_len3d(&d);

        if (dist < 1.0f || dist > max_dist)
            continue;

        sh_normalize(&d);

        f32 dot = sh_dot(&d, &fwd);

        if (dot < 0.0f)
            continue;

        f32 angle = sh_rad2deg * sh_acos(dot);

        if (angle > best_fov)
            continue;

        out->candidates++;

        qbool visible = sh_player_visible(p);
        f32 dealt = 0.0f;

        if (visible)
        {
            dealt = (f32)(p->health > 0 ? p->health : 100);
        }
        else if (g_cfg.aimbot.auto_wall || g_cfg.aimbot.penetration)
        {
            sh_autowall_ctx_t aw;
            aw.start = eye;
            aw.end = center;
            const sh_weapon_ballistic_t *wb = sh_weapon_ballistic_for(local_p->weapon_id);

            if (!wb || wb->damage <= 0.0f)
                continue;

            aw.bullet_id = 1;
            aw.penetration = wb->penetration;
            aw.base_damage = wb->damage;
            aw.range_modifier = wb->range_modifier;
            aw.penetrate_world = g_cfg.aimbot.penetration;
            aw.trace = sh_rage_trace;
            aw.user = 0;

            dealt = sh_autowall_damage(&aw);
        }

        if (g_cfg.aimbot.visible_check && !visible)
            continue;

        if (g_cfg.aimbot.smoke_check && sh_player_behind_smoke(p))
            continue;

        if (dealt <= 0.0f)
            continue;

        f32 score = 0.0f;

        switch (g_cfg.aimbot.priority)
        {
        case 1:
            score = angle;
            break;
        case 2:
            score = 100000.0f - (f32)(p->health > 0 ? p->health : 100);
            break;
        case 3:
            score = angle * 1000.0f + dist;
            break;
        default:
            score = dist;
            break;
        }

        if (!out->found || score < best_score)
        {
            best_score = score;
            out->found = 1;
            out->target.index = p->index;
            out->target.distance = dist;
            out->target.aim_point = center;
            out->target.hitgroup = hit_grp;
            out->target.damage = dealt;
            out->target.through_wall = visible ? 0 : 1;
        }
    }
}

qbool sh_rage_should_fire(const sh_rage_result_t *r)
{
    if (!r || !r->found)
        return 0;

    if (r->target.damage < g_cfg.aimbot.min_damage)
        return 0;

    if (g_cfg.aimbot.auto_fire)
        return 1;

    if (g_cfg.aimbot.key && (GetAsyncKeyState(g_cfg.aimbot.key) & 0x8000))
        return 1;

    return 0;
}

void sh_rage_apply(sh_usercmd_t *cmd, const sh_rage_result_t *r)
{
    if (!cmd || !r || !r->found)
        return;

    sh_player_t *local = sh_player_local();

    if (!local)
        return;

    vec3_t eye = local->origin;
    eye.z += 28.0f;

    vec3_t d = r->target.aim_point;
    d.x -= eye.x;
    d.y -= eye.y;
    d.z -= eye.z;

    vec3_t want;
    sh_vectors_angle(&d, &want);

    f32 smooth = g_cfg.aimbot.smooth;

    if (smooth < 1.0f)
        smooth = 1.0f;

    if (g_cfg.aimbot.silent)
    {
        cmd->viewangles = want;
    }
    else
    {
        cmd->viewangles.x += sh_angle_delta(want.x, cmd->viewangles.x) / smooth;
        cmd->viewangles.y += sh_angle_delta(want.y, cmd->viewangles.y) / smooth;
    }

    if (g_cfg.aimbot.auto_stop && sh_rage_should_fire(r))
    {
        f32 vx = g_pmove ? g_pmove->velocity.x : 0.0f;
        f32 vy = g_pmove ? g_pmove->velocity.y : 0.0f;
        f32 speed = sh_sqrt(vx * vx + vy * vy);

        if (speed > sh_autostop_threshold)
        {
            cmd->forwardmove = 0.0f;
            cmd->sidemove = 0.0f;
            cmd->upmove = 0.0f;

            if (g_cfg.aimbot.auto_stop_duck && g_pmove &&
                g_pmove->onground != -1)
                cmd->buttons |= sh_in_duck;
        }
    }

    if (g_cfg.aimbot.auto_scope && g_pmove)
    {
        i32 wid = local->weapon_id;

        if (wid == 18 || wid == 3 || wid == 24 || wid == 13)
        {
            if (g_local_fov > 90.0f)
                cmd->buttons |= sh_in_attack2;
        }
    }

    if (sh_rage_should_fire(r))
    {
        f32 now = g_pmove ? g_pmove->time : 0.0f;
        f32 npa = g_pmove ? g_pmove->next_primary_attack : 0.0f;

        if (g_cfg.aimbot.double_tap)
            sh_rage_double_tap(cmd, npa, now);
        else
            cmd->buttons |= sh_in_attack;

        if (g_cfg.visuals.tracer)
        {
            vec3_t from = eye;
            vec3_t dir = d;

            sh_normalize(&dir);

            f32 sx = 0.0f, sy = 0.0f;
            i32 seed = (i32)((g_pmove ? g_pmove->time : 0.0f) * 1000.0f);

            sh_spread_angle(local->weapon_id, seed, &sx, &sy);

            if (sx != 0.0f || sy != 0.0f)
            {
                vec3_t right, up;
                sh_angle_vectors(&cmd->viewangles, &dir, &right, &up);

                vec3_t spread_dir;
                sh_spread_trace_dir(&dir, &right, &up, sx, sy, &spread_dir);

                dir = spread_dir;
            }

            vec3_t to;
            to.x = from.x + dir.x * 8192.0f;
            to.y = from.y + dir.y * 8192.0f;
            to.z = from.z + dir.z * 8192.0f;

            vec3_t end;
            qbool hit = sh_trace_line(&from, &to, &end);

            if (hit)
                to = end;

            sh_visuals_tracer_add(&from, &to);
        }

        if (g_cfg.visuals.impact)
        {
            vec3_t dir = d;
            sh_normalize(&dir);

            vec3_t to;
            to.x = eye.x + dir.x * 8192.0f;
            to.y = eye.y + dir.y * 8192.0f;
            to.z = eye.z + dir.z * 8192.0f;

            vec3_t end;

            if (sh_trace_line(&eye, &to, &end))
            {
                vec3_t norm = { 0.0f, 0.0f, 1.0f };
                norm.x = eye.x - end.x;
                norm.y = eye.y - end.y;
                norm.z = eye.z - end.z;
                sh_normalize(&norm);

                sh_visuals_impact_add(&end, &norm);
            }
        }

        {
            f32 vt = g_pmove ? g_pmove->time : 0.0f;
            sh_hitmarker_tick(vt);
        }
    }
}

qbool sh_rage_double_tap(sh_usercmd_t *cmd, f32 next_attack_time, f32 now)
{
    if (!cmd || !g_cfg.aimbot.double_tap)
        return 0;

    f32 frametime = g_pmove ? g_pmove->frametime : 0.01f;

    if (frametime <= 0.0f)
        frametime = 0.01f;

    if (next_attack_time > now + frametime)
        return 0;

    if (!(cmd->buttons & sh_in_attack))
        cmd->buttons |= sh_in_attack;

    g_last_attack_sent = now;

    return 1;
}

void sh_rage_run(sh_usercmd_t *cmd)
{
    if (!cmd || !g_cfg.aimbot.enabled)
        return;

    sh_rage_scan(&g_rage);

    if (!g_rage.found)
        return;

    sh_rage_apply(cmd, &g_rage);
}

void sh_rage_apply_recoil(sh_usercmd_t *cmd)
{
    if (!cmd || !g_pmove)
        return;

    if (!g_cfg.misc.no_recoil && !g_cfg.misc.no_spread)
        return;

    if (g_cfg.misc.no_recoil)
    {
        cmd->viewangles.x -= g_pmove->punchangle.x;
        cmd->viewangles.y -= g_pmove->punchangle.y;
    }

    if (g_cfg.misc.no_spread)
    {
        sh_player_t *lp = sh_player_local();
        i32 wid = lp ? lp->weapon_id : 0;

        u32 seed = sh_spread_seed();

        sh_nospread_angles(wid, seed, 0, 1, &cmd->viewangles);
    }
}
