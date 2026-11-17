#include "../../include/sh_rage.h"
#include "../../include/sh_config.h"
#include "../../include/sh_math.h"
#include "../../include/sh_features.h"
#include "../../include/sh_visuals.h"
#include "../../include/sh_hitmarker.h"

#include <windows.h>


static sh_rage_result_t g_rage;
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

        if (!out->found || dist < out->target.distance)
        {
            out->found = 1;
            out->target.index = p->index;
            out->target.distance = dist;
            out->target.aim_point = center;
            out->target.hitgroup = 2;
            out->target.damage = 100.0f;
            out->target.through_wall = 0;
            best_fov = angle;
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

    if (sh_rage_should_fire(r))
    {
        cmd->buttons |= 1;

        if (g_cfg.visuals.tracer)
        {
            vec3_t from = eye;
            vec3_t to = r->target.aim_point;
            sh_visuals_tracer_add(&from, &to);
        }

        {
            f32 vt = g_pmove ? g_pmove->time : 0.0f;
            vec3_t d2 = r->target.aim_point;
            (void)d2;
            sh_hitmarker_on_hit(r->target.hitgroup == 1, (i32)r->target.damage);
            sh_hitmarker_tick(vt);
        }
    }
}

qbool sh_rage_double_tap(sh_usercmd_t *cmd, f32 next_attack_time, f32 now)
{
    if (!cmd || !g_cfg.aimbot.double_tap)
        return 0;

    if (next_attack_time <= now)
        return 0;

    if (now - g_last_attack_sent < 0.25f)
        return 0;

    cmd->buttons &= ~1;
    cmd->buttons |= 4;

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
