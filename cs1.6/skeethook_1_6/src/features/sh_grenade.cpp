#include "../../include/sh_grenade.h"
#include "../../include/sh_math.h"

static const f32 g_grenade_step = 0.05f;

f32 sh_grenade_launch_speed(f32 pitch)
{
    f32 a = pitch;

    if (a < 0.0f)
        a = -10.0f + a * ((90.0f - 10.0f) / 90.0f);
    else
        a = -10.0f + a * ((90.0f + 10.0f) / 90.0f);

    f32 vel = (90.0f - a) * 6.0f;

    if (vel > sh_grenade_max_speed)
        vel = sh_grenade_max_speed;

    return vel;
}

void sh_grenade_predict(const vec3_t *origin, const vec3_t *angles,
                        const vec3_t *player_velocity, sh_grenade_path_t *out)
{
    if (!origin || !angles || !out)
        return;

    out->count = 0;
    out->hit = 0;

    vec3_t fwd, right, up;
    sh_angle_vectors(angles, &fwd, &right, &up);

    f32 speed = sh_grenade_launch_speed(angles->x);
    out->launch_speed = speed;

    vec3_t pos = *origin;
    pos.x += fwd.x * 16.0f;
    pos.y += fwd.y * 16.0f;
    pos.z += fwd.z * 16.0f;

    vec3_t vel;

    vel.x = fwd.x * speed;
    vel.y = fwd.y * speed;
    vel.z = fwd.z * speed;

    if (player_velocity)
    {
        vel.x += player_velocity->x;
        vel.y += player_velocity->y;
        vel.z += player_velocity->z;
    }

    for (i32 i = 0; i < sh_grenade_max_points; ++i)
    {
        out->points[i] = pos;
        out->count = i + 1;

        vel.z -= sh_grenade_gravity * g_grenade_step;

        pos.x += vel.x * g_grenade_step;
        pos.y += vel.y * g_grenade_step;
        pos.z += vel.z * g_grenade_step;

        if (pos.z < -4096.0f)
            break;
    }

    out->impact = pos;
    out->hit = out->count > 0;
}

void sh_grenade_damage_radius(const vec3_t *center, const vec3_t *victim,
                              f32 base_damage, f32 radius, f32 *out)
{
    if (!center || !victim || !out)
        return;

    vec3_t d;

    d.x = victim->x - center->x;
    d.y = victim->y - center->y;
    d.z = victim->z - center->z;

    f32 dist = sh_len3d(&d);

    if (dist >= radius)
    {
        *out = 0.0f;
        return;
    }

    *out = base_damage * (1.0f - dist / radius);
}

f32 sh_grenade_range(f32 theta_up, f32 start_z)
{
    f32 v = sh_grenade_launch_speed(-theta_up);

    if (v <= 0.0f)
        return 0.0f;

    f32 rad = theta_up * (sh_pi / 180.0f);
    f32 vx = v * sh_cos(rad);
    f32 vz = v * sh_sin(rad);
    f32 x = 0.0f;
    f32 z = start_z;

    for (i32 i = 0; i < 2000 && z >= 0.0f; ++i)
    {
        vz -= sh_grenade_gravity * 0.01f;
        x += vx * 0.01f;
        z += vz * 0.01f;
    }

    return x;
}

f32 sh_grenade_optimal_angle(void)
{
    f32 best_x = 0.0f;
    f32 best_a = 0.0f;

    for (f32 a = 0.0f; a < 90.0f; a += 1.0f)
    {
        f32 x = sh_grenade_range(a, 64.0f);

        if (x > best_x)
        {
            best_x = x;
            best_a = a;
        }
    }

    return best_a;
}
