#include "../../include/sh_features.h"
#include "../../include/sh_math.h"

#include <math.h>
#include <windows.h>

static sh_target_t g_target;
static f32 g_aim_time;

static const f32 g_hitbox_offsets[] = {
    28.0f,
    26.0f,
    18.0f,
    8.0f,
    2.0f,
    -6.0f
};

typedef struct sh_hitgroup_s
{
    const char *name;
    f32 damage_mult;
} sh_hitgroup_t;

static const sh_hitgroup_t g_hitgroups[] = {
    {"generic", 1.0f},
    {"head", 4.0f},
    {"chest", 1.0f},
    {"stomach", 1.25f},
    {"leftarm", 1.0f},
    {"rightarm", 1.0f},
    {"leftleg", 0.75f},
    {"rightleg", 0.75f},
};

f32 sh_hitgroup_damage_mult(i32 group)
{
    if (group < 0 || group >= (i32)sh_countof(g_hitgroups))
        return 1.0f;
    return g_hitgroups[group].damage_mult;
}

static void sh_aim_point(const sh_player_t *p, i32 hitbox, vec3_t *out)
{
    *out = p->origin;
    out->z += g_hitbox_offsets[sh_clampi(hitbox, 0, (i32)sh_countof(g_hitbox_offsets) - 1)];
}

static f32 sh_fov_to(const sh_player_t *p, const vec3_t *eye, const vec3_t *view_angles)
{
    vec3_t dir;
    sh_sub(&dir, &p->origin, eye);
    sh_normalize(&dir);

    vec3_t fwd;
    sh_angle_vectors(view_angles, &fwd, 0, 0);

    f32 d = sh_dot(&fwd, &dir);
    if (d > 1.0f) d = 1.0f;
    if (d < -1.0f) d = -1.0f;
    return sh_deg(acosf(d));
}

static qbool sh_can_hit(const sh_player_t *p, const vec3_t *eye, const vec3_t *point, f32 min_damage)
{
    vec3_t dir;
    sh_sub(&dir, point, eye);
    f32 dist = sh_len3d(&dir);
    if (dist < 1.0f)
        return true;

    sh_normalize(&dir);

    vec3_t end;
    sh_scale(&end, &dir, dist + 32.0f);
    sh_add(&end, eye, &end);

    if (!g_trace_fn)
        return true;

    i32 hit = g_trace_fn(*eye, end, 0, p->index);

    if (!hit)
        return false;

    f32 base = 30.0f;
    f32 range_mod = 0.98f;
    f32 dmg = base * powf(range_mod, dist);

    if (dmg < min_damage)
        return false;

    return true;
}

void sh_aim_select_target(sh_target_t *out)
{
    out->valid = false;
    out->fov = 9999.0f;

    if (!g_pmove)
        return;

    vec3_t eye = g_pmove->origin;
    eye.z += (g_pmove->flags & sh_fl_ducking) ? 12.0f : 28.0f;

    vec3_t view = g_pmove->angles;
    if (g_engfuncs && g_engfuncs->pfnGetViewAngles)
        g_engfuncs->pfnGetViewAngles((f32 *)&view);

    i32 count = sh_players_count();
    for (i32 i = 0; i < count; ++i)
    {
        sh_player_t *p = sh_player_get(i);
        if (!p || !p->ent)
            continue;
        if (p->health <= 0)
            continue;

        if (g_cfg.esp.enemy_only && p->team == g_cfg_local_team)
            continue;

        if (p->distance > g_cfg.visuals.max_distance)
            continue;

        vec3_t point;
        sh_aim_point(p, g_cfg.aimbot.hitbox, &point);

        if (g_cfg.aimbot.visible_check && !p->visible)
            continue;

        f32 fov = sh_fov_to(p, &eye, &view);
        if (fov > g_cfg.aimbot.fov)
            continue;

        if (g_cfg.aimbot.auto_wall && !sh_can_hit(p, &eye, &point, g_cfg.aimbot.min_damage))
            continue;

        if (fov < out->fov)
        {
            out->valid = true;
            out->fov = fov;
            out->player = *p;
            out->hitbox = g_cfg.aimbot.hitbox;
            out->distance = p->distance;
            out->point = point;
        }
    }
}

void sh_aim_lock_angles(sh_target_t *target, vec3_t *angles)
{
    if (!target || !target->valid || !g_pmove)
        return;

    vec3_t eye = g_pmove->origin;
    eye.z += (g_pmove->flags & sh_fl_ducking) ? 12.0f : 28.0f;

    vec3_t delta;
    sh_sub(&delta, &target->point, &eye);

    vec3_t want;
    sh_vectors_angle(&delta, &want);

    f32 smooth = g_cfg.aimbot.smooth;
    if (smooth < 1.0f)
        smooth = 1.0f;

    angles->x += sh_angle_delta(want.x, angles->x) / smooth;
    angles->y += sh_angle_delta(want.y, angles->y) / smooth;
    sh_angle_normalize(angles);
}

void sh_aim_run(sh_usercmd_t *cmd)
{
    g_target.valid = false;

    if (!g_cfg.aimbot.enabled || !g_pmove)
        return;

    qbool active = true;
    if (g_cfg.aimbot.key)
        active = (GetAsyncKeyState(g_cfg.aimbot.key) & 0x8000) != 0;

    if (!active)
        return;

    sh_aim_select_target(&g_target);
    if (!g_target.valid)
        return;

    sh_aim_lock_angles(&g_target, &cmd->viewangles);

    if (g_cfg.aimbot.auto_fire)
        cmd->buttons |= sh_in_attack;
}

sh_target_t *sh_aim_target(void)
{
    return &g_target;
}
