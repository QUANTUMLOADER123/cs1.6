#include "../../include/sh_knife.h"
#include "../../include/sh_config.h"
#include "../../include/sh_math.h"
#include "../../include/sh_features.h"

qbool sh_knife_should_stab(const vec3_t *target, const vec3_t *eye,
                           const vec3_t *view_angles, f32 distance)
{
    if (!target || !eye || !view_angles)
        return 0;

    if (distance > sh_knife_stab_distance)
        return 0;

    vec3_t d;

    d.x = target->x - eye->x;
    d.y = target->y - eye->y;
    d.z = target->z - eye->z;

    vec3_t fwd, right, up;
    sh_angle_vectors(view_angles, &fwd, &right, &up);

    sh_normalize(&d);

    return sh_dot(&d, &fwd) > 0.85f;
}

void sh_knife_run(sh_usercmd_t *cmd)
{
    if (!cmd || !g_cfg.misc.knife_bot)
        return;

    sh_player_t *local = sh_player_local();

    if (!local)
        return;

    sh_player_t *best = 0;
    f32 best_dist = 999999.0f;

    for (i32 i = 1; i <= 32; ++i)
    {
        sh_player_t *p = sh_player_get(i);

        if (!p || !p->ent || p->index == g_local_player)
            continue;

        if (local->team && p->team == local->team)
            continue;

        if (p->health <= 0)
            continue;

        if (!p->visible)
            continue;

        if (p->distance < best_dist)
        {
            best_dist = p->distance;
            best = p;
        }
    }

    if (!best)
        return;

    if (best_dist > sh_knife_swing_distance + 16.0f)
        return;

    vec3_t eye = local->origin;
    eye.z += 28.0f;

    vec3_t aim = best->origin;
    aim.z += 36.0f;

    vec3_t d;

    d.x = aim.x - eye.x;
    d.y = aim.y - eye.y;
    d.z = aim.z - eye.z;

    vec3_t want;
    sh_vectors_angle(&d, &want);

    cmd->viewangles = want;

    cmd->buttons |= sh_in_attack;
}
