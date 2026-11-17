#include "../../include/sh_rage.h"
#include "../../include/sh_config.h"
#include "../../include/sh_math.h"
#include "../../include/sh_features.h"
#include "../../include/sh_visuals.h"
#include "../../include/sh_hitmarker.h"
#include "../../include/sh_worldfx.h"
#include "../../include/sh_autowall.h"
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

    if (!sh_trace_line(start, end, 0))
    {
        out->fraction = 0.0f;
        out->entity = 0;
    }

    return 1;
}
static f32 g_last_attack_sent;


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

        if (local_p->team && p->team == local_p->team)
            continue;

        if (p->health <= 0)
            continue;

        vec3_t center = p->origin;
        center.z += 36.0f;

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
            aw.bullet_id = 1;
            aw.penetration = 2;
            aw.base_damage = 30.0f;
            aw.range_modifier = 0.98f;
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
            out->target.hitgroup = g_cfg.aimbot.hitbox;
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
        cmd->forwardmove = 0.0f;
        cmd->sidemove = 0.0f;
        cmd->buttons &= ~2;
    }

    if (sh_rage_should_fire(r))
    {
        f32 now = g_pmove ? g_pmove->time : 0.0f;
        f32 npa = g_pmove ? g_pmove->next_primary_attack : 0.0f;

        if (g_cfg.aimbot.double_tap)
            sh_rage_double_tap(cmd, npa, now);
        else
            cmd->buttons |= 1;

        if (g_cfg.visuals.tracer)
        {
            vec3_t from = eye;
            vec3_t to = r->target.aim_point;

            const sh_recoil_t *rec = sh_recoil_for(local->weapon_id);

            if (rec && rec->up_base > 0.0f)
            {
                vec2_t sp;
                i32 seed = (i32)(now * 1000.0f) ^ (i32)GetTickCount();
                sh_spread_compute(seed, rec->up_base * 10.0f, &sp);

                to.x += sp.x;
                to.y += sp.y;
            }

            sh_visuals_tracer_add(&from, &to);
        }

        if (g_cfg.visuals.impact)
        {
            vec3_t norm = { 0.0f, 0.0f, 1.0f };
            sh_visuals_impact_add(&r->target.aim_point, &norm);
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

    if (!(cmd->buttons & 1))
        cmd->buttons |= 1;

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
        sh_spread_compensate(&cmd->viewangles);
}
