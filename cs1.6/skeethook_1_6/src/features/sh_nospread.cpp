#include "../../include/sh_ballistics.h"
#include "../../include/sh_config.h"
#include "../../include/sh_math.h"
#include "../../include/sh_sdk.h"
#include "../../include/sh_features.h"
#include "../../include/sh_nospread.h"

typedef struct sh_wpn_spread_s
{
    i32 id;
    f32 air_base;
    f32 air_mul;
    f32 run_base;
    f32 run_mul;
    f32 duck_mul;
    f32 stand_mul;
    f32 base_mul;
    f32 run_threshold;
    f32 zoom_bonus;
    f32 extra_bonus;
    u8 mode;
    u8 flags;
} sh_wpn_spread_t;

#define sh_sp_mode_inv 1
#define sh_sp_mode_mul 2
#define sh_sp_flag_zoom  0x01
#define sh_sp_flag_burst 0x02
#define sh_sp_flag_silent 0x04

static const sh_wpn_spread_t g_spread_tbl[] =
{
    { 28, 0.040f, 0.400f, 0.040f, 0.070f, 0.0275f, 0.0275f, 0.0275f, 140.0f, 0.0f, 0.0f, sh_sp_mode_mul, 0 },
    { 22, 0.035f, 0.400f, 0.035f, 0.070f, 0.025f,  0.020f,  0.020f,  140.0f, 0.0f, 0.0f, sh_sp_mode_mul, 0 },
    { 27, 0.035f, 0.450f, 0.035f, 0.075f, 0.020f,  0.020f,  0.020f,  140.0f, 0.0f, 0.0f, sh_sp_mode_mul, 0 },
    {  8, 0.035f, 0.400f, 0.035f, 0.070f, 0.020f,  0.020f,  0.020f,  140.0f, 0.0f, 0.0f, sh_sp_mode_mul, sh_sp_flag_zoom },
    { 14, 0.040f, 0.300f, 0.040f, 0.070f, 0.0375f, 0.0375f, 0.0375f, 140.0f, 0.0f, 0.0f, sh_sp_mode_mul, 0 },
    { 15, 0.030f, 0.300f, 0.030f, 0.070f, 0.020f,  0.020f,  0.020f,  140.0f, 0.0f, 0.01f, sh_sp_mode_mul, sh_sp_flag_burst },
    { 16, 0.055f, 0.500f, 0.055f, 0.100f, 0.035f,  0.035f,  0.035f,  140.0f, 0.0f, 0.0f, sh_sp_mode_mul, 0 },
    { 19, 0.040f, 0.200f, 0.040f, 0.040f, 0.040f,  0.040f,  0.040f,  140.0f, 0.0f, 0.0f, sh_sp_mode_mul, 0 },
    { 17, 0.080f, 0.200f, 0.080f, 0.040f, 0.040f,  0.040f,  0.040f,  140.0f, 0.0f, 0.0f, sh_sp_mode_mul, sh_sp_flag_silent },
    {  2, 0.080f, 0.200f, 0.080f, 0.040f, 0.040f,  0.040f,  0.040f,  140.0f, 0.0f, 0.0f, sh_sp_mode_mul, sh_sp_flag_silent },
    { 26, 0.060f, 0.350f, 0.060f, 0.060f, 0.030f,  0.030f,  0.030f,  140.0f, 0.0f, 0.0f, sh_sp_mode_mul, 0 },

    {  1, 1.500f, 0.0f, 0.255f, 0.0f, 0.075f, 0.150f, 0.0f, 0.0f, 0.0f, 0.0f, sh_sp_mode_inv, 0 },
    { 11, 1.500f, 0.0f, 0.255f, 0.0f, 0.075f, 0.150f, 0.0f, 0.0f, 0.0f, 0.0f, sh_sp_mode_inv, 0 },
    { 10, 1.300f, 0.0f, 0.175f, 0.0f, 0.080f, 0.100f, 0.0f, 0.0f, 0.0f, 0.0f, sh_sp_mode_inv, 0 },
    { 26, 1.500f, 0.0f, 0.250f, 0.0f, 0.115f, 0.130f, 0.0f, 0.0f, 0.0f, 0.0f, sh_sp_mode_inv, 0 },
    { 16, 1.300f, 0.0f, 0.250f, 0.0f, 0.125f, 0.150f, 0.0f, 0.0f, 0.0f, 0.0f, sh_sp_mode_inv, sh_sp_flag_silent },

    {  3, 0.200f, 0.0f, 0.075f, 0.0f, 0.000f, 0.007f, 0.0f, 170.0f, 0.025f, 0.0f, 0, sh_sp_flag_zoom },
    { 18, 0.850f, 0.0f, 0.250f, 0.0f, 0.000f, 0.001f, 0.0f, 140.0f, 0.080f, 0.0f, 0, 0 },
    { 13, 0.450f, 0.0f, 0.150f, 0.0f, 0.040f, 0.050f, 0.0f, 0.0f,   0.025f, 0.0f, 0, sh_sp_flag_zoom },
    { 24, 0.450f, 0.0f, 0.150f, 0.0f, 0.035f, 0.055f, 0.0f, 0.0f,   0.025f, 0.0f, 0, sh_sp_flag_zoom },
    { 20, 0.0f,   0.0f, 0.0f,   0.0f, 0.0f,   0.0f,   0.0f, 0.0f,   0.0f,   0.0f, 0, 0 },
    { 21, 0.0f,   0.0f, 0.0f,   0.0f, 0.0f,   0.0f,   0.0f, 0.0f,   0.0f,   0.0f, 0, 0 },
    {  5, 0.0f,   0.0f, 0.0f,   0.0f, 0.0f,   0.0f,   0.0f, 0.0f,   0.0f,   0.0f, 0, 0 }
};

static const sh_wpn_spread_t *sh_spread_tbl_for(i32 id)
{
    for (i32 i = 0; i < (i32)(sizeof(g_spread_tbl) / sizeof(g_spread_tbl[0])); ++i)
    {
        if (g_spread_tbl[i].id == id)
            return &g_spread_tbl[i];
    }

    return 0;
}

typedef struct sh_acc_state_s
{
    f32 accuracy;
    i32 shots;
    i32 weapon_id;
    f32 last_shot;
    f32 last_check;
} sh_acc_state_t;

static sh_acc_state_t g_acc;

static f32 sh_acc_init_for(i32 id)
{
    switch (id)
    {
    case 28:
    case 8:
    case 15:
    case 14:
    case 27:
    case 22:
    case 20:
    case 30:
    case 23:
    case 7:
        return 0.2f;
    case 26:
    case 17:
    case 1:
        return 0.9f;
    case 10:
        return 0.88f;
    case 11:
        return 0.92f;
    case 16:
        return 0.92f;
    case 2:
        return 0.9f;
    case 19:
        return 0.9f;
    default:
        return 0.0f;
    }
}

void sh_nospread_reset(void)
{
    g_acc.accuracy = 0.0f;
    g_acc.shots = 0;
    g_acc.weapon_id = 0;
    g_acc.last_shot = 0.0f;
    g_acc.last_check = 0.0f;
}

static f32 sh_now(void)
{
    return g_pmove ? g_pmove->time : 0.0f;
}

static void sh_acc_shot(i32 id)
{
    f32 now = sh_now();

    if (g_acc.weapon_id != id)
    {
        g_acc.weapon_id = id;
        g_acc.shots = 0;
        g_acc.accuracy = sh_acc_init_for(id);
        g_acc.last_shot = 0.0f;
    }

    switch (id)
    {
    case 28:
        if (++g_acc.shots > 1)
        {
            f32 s = (f32)g_acc.shots;
            g_acc.accuracy = (s * s * s) / 200.0f + 0.35f;
            if (g_acc.accuracy > 1.25f) g_acc.accuracy = 1.25f;
        }
        break;

    case 8:
        if (++g_acc.shots > 1)
        {
            f32 s = (f32)g_acc.shots;
            g_acc.accuracy = (s * s * s) / 220.0f + 0.3f;
            if (g_acc.accuracy > 1.0f) g_acc.accuracy = 1.0f;
        }
        break;

    case 13:
    case 24:
        if (++g_acc.shots > 1)
        {
            f32 s = (f32)g_acc.shots;
            g_acc.accuracy = (s * s) / 210.0f + 0.5f;
            if (g_acc.accuracy > 1.0f) g_acc.accuracy = 1.0f;
        }
        break;

    case 23:
        if (++g_acc.shots > 1)
        {
            f32 s = (f32)g_acc.shots;
            g_acc.accuracy = (s * s * s) / 200.0f + 0.55f;
            if (g_acc.accuracy > 1.4f) g_acc.accuracy = 1.4f;
        }
        break;

    case 14:
        if (++g_acc.shots > 1)
        {
            f32 s = (f32)g_acc.shots;
            g_acc.accuracy = (s * s * s) / 200.0f + 0.35f;
            if (g_acc.accuracy > 1.25f) g_acc.accuracy = 1.25f;
        }
        break;

    case 20:
        if (++g_acc.shots > 1)
        {
            f32 s = (f32)g_acc.shots;
            g_acc.accuracy = (s * s * s) / 175.0f + 0.45f;
            if (g_acc.accuracy > 1.0f) g_acc.accuracy = 1.0f;
        }
        break;

    case 19:
        if (++g_acc.shots > 1)
        {
            f32 s = (f32)g_acc.shots;
            g_acc.accuracy = (s * s) / 210.0f + 0.5f;
            if (g_acc.accuracy > 1.0f) g_acc.accuracy = 1.0f;
        }
        break;

    case 22:
        if (++g_acc.shots > 1)
        {
            f32 s = (f32)g_acc.shots;
            g_acc.accuracy = (s * s) / 175.0f + 0.45f;
            if (g_acc.accuracy > 1.0f) g_acc.accuracy = 1.0f;
        }
        break;

    case 12:
        if (++g_acc.shots > 1)
        {
            f32 s = (f32)g_acc.shots;
            g_acc.accuracy = (s * s) / 210.0f + 0.5f;
            if (g_acc.accuracy > 1.0f) g_acc.accuracy = 1.0f;
        }
        break;

    case 7:
        if (++g_acc.shots > 1)
        {
            f32 s = (f32)g_acc.shots;
            g_acc.accuracy = (s * s) / 200.0f + 0.4f;
            if (g_acc.accuracy > 1.65f) g_acc.accuracy = 1.65f;
        }
        break;

    case 30:
        if (++g_acc.shots > 1)
        {
            f32 s = (f32)g_acc.shots;
            g_acc.accuracy = (s * s) / 175.0f + 0.45f;
            if (g_acc.accuracy > 1.0f) g_acc.accuracy = 1.0f;
        }
        break;

    case 16:
    case 2:
    case 1:
    case 11:
    case 17:
    case 26:
    case 10:
        if (g_acc.shots < 1)
            g_acc.shots = 1;
        break;

    default:
        ++g_acc.shots;
        break;
    }

    g_acc.last_shot = now;
}

void sh_nospread_on_fire(i32 weapon_id)
{
    sh_acc_shot(weapon_id);
}

f32 sh_nospread_accuracy(void)
{
    return g_acc.accuracy;
}

f32 sh_nospread_spread(i32 weapon_id, qbool silenced, qbool in_burst,
                       i32 fov)
{
    const sh_wpn_spread_t *t = sh_spread_tbl_for(weapon_id);

    if (!t)
        return 0.0f;

    f32 acc = g_acc.accuracy;
    f32 spread = 0.0f;

    if (t->mode == sh_sp_mode_inv)
        spread = t->base_mul;
    else
        spread = t->base_mul;

    qbool onground = 1;
    f32 speed = 0.0f;
    qbool ducking = 0;

    if (g_pmove)
    {
        onground = (g_pmove->onground != -1) ? 1 : 0;

        f32 vx = g_pmove->velocity.x;
        f32 vy = g_pmove->velocity.y;
        speed = sh_sqrt(vx * vx + vy * vy);

        ducking = (g_pmove->usehull == 1) ? 1 : 0;
    }

    if (t->mode == sh_sp_mode_inv)
    {
        if (!onground)
            spread = t->air_base * (1.0f - acc);
        else if (speed > 0.0f)
            spread = t->run_base * (1.0f - acc);
        else if (ducking)
            spread = t->duck_mul * (1.0f - acc);
        else
            spread = t->stand_mul * (1.0f - acc);
    }
    else if (t->mode == sh_sp_mode_mul)
    {
        if (!onground)
            spread = t->air_base + acc * t->air_mul;
        else if (speed > t->run_threshold)
            spread = t->run_base + acc * t->run_mul;
        else
            spread = acc * t->stand_mul;
    }
    else
    {
        if (!onground)
            spread = t->air_base;
        else if (speed > t->run_threshold)
            spread = t->run_base;
        else if (ducking)
            spread = t->duck_mul;
        else
            spread = t->stand_mul;

        if (t->zoom_bonus > 0.0f && fov == 90)
            spread += t->zoom_bonus;
    }

    if ((t->flags & sh_sp_flag_burst) && !in_burst)
        spread += t->extra_bonus;

    if (spread < 0.0f)
        spread = 0.0f;

    return spread;
}

void sh_nospread_angles(i32 weapon_id, u32 seed, qbool silenced,
                        qbool in_burst, vec3_t *viewangles)
{
    if (!viewangles)
        return;

    f32 spread = sh_nospread_spread(weapon_id, silenced, in_burst, 90);

    if (spread <= 0.0001f)
        return;

    f32 sx = sh_shared_random_float(seed + 1u, -0.5f, 0.5f)
           + sh_shared_random_float(seed + 2u, -0.5f, 0.5f);

    f32 sy = sh_shared_random_float(seed + 3u, -0.5f, 0.5f)
           + sh_shared_random_float(seed + 4u, -0.5f, 0.5f);

    sx *= spread;
    sy *= spread;

    vec3_t fwd, right, up;
    sh_angle_vectors(viewangles, &fwd, &right, &up);

    vec3_t dir;
    dir.x = fwd.x + sx * right.x + sy * up.x;
    dir.y = fwd.y + sx * right.y + sy * up.y;
    dir.z = fwd.z + sx * right.z + sy * up.z;

    sh_normalize(&dir);

    vec3_t want;
    sh_vectors_angle(&dir, &want);

    viewangles->x = want.x;
    viewangles->y = want.y;
}

f32 sh_nospread_radius(i32 weapon_id)
{
    return sh_nospread_spread(weapon_id, 0, 1, 90) * 1000.0f;
}
