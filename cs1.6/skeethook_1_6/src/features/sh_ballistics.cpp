#include "../../include/sh_ballistics.h"
#include "../../include/sh_movement.h"
#include "../../include/sh_math.h"
#include "../../include/sh_sdk.h"

static const u32 g_seed_table[256] =
{
    28985u, 27138u, 26457u, 9451u, 17764u, 10909u, 28790u, 8716u, 6361u, 4853u, 17798u, 21977u,
    19643u, 20662u, 10834u, 20103u, 27067u, 28634u, 18623u, 25849u, 8576u, 26234u, 23887u, 18228u,
    32587u, 4836u, 3306u, 1811u, 3035u, 24559u, 18399u, 315u, 26766u, 907u, 24102u, 12370u,
    9674u, 2972u, 10472u, 16492u, 22683u, 11529u, 27968u, 30406u, 13213u, 2319u, 23620u, 16823u,
    10013u, 23772u, 21567u, 1251u, 19579u, 20313u, 18241u, 30130u, 8402u, 20807u, 27354u, 7169u,
    21211u, 17293u, 5410u, 19223u, 10255u, 22480u, 27388u, 9946u, 15628u, 24389u, 17308u, 2370u,
    9530u, 31683u, 25927u, 23567u, 11694u, 26397u, 32602u, 15031u, 18255u, 17582u, 1422u, 28835u,
    23607u, 12597u, 20602u, 10138u, 5212u, 1252u, 10074u, 23166u, 19823u, 31667u, 5902u, 24630u,
    18948u, 14330u, 14950u, 8939u, 23540u, 21311u, 22428u, 22391u, 3583u, 29004u, 30498u, 18714u,
    4278u, 2437u, 22430u, 3439u, 28313u, 23161u, 25396u, 13471u, 19324u, 15287u, 2563u, 18901u,
    13103u, 16867u, 9714u, 14322u, 15197u, 26889u, 19372u, 26241u, 31925u, 14640u, 11497u, 8941u,
    10056u, 6451u, 28656u, 10737u, 13874u, 17356u, 8281u, 25937u, 1661u, 4850u, 7448u, 12744u,
    21826u, 5477u, 10167u, 16705u, 26897u, 8839u, 30947u, 27978u, 27283u, 24685u, 32298u, 3525u,
    12398u, 28726u, 9475u, 10208u, 617u, 13467u, 22287u, 2376u, 6097u, 26312u, 2974u, 9114u,
    21787u, 28010u, 4725u, 15387u, 3274u, 10762u, 31695u, 17320u, 18324u, 12441u, 16801u, 27376u,
    22464u, 7500u, 5666u, 18144u, 15314u, 31914u, 31627u, 6495u, 5226u, 31203u, 2331u, 4668u,
    12650u, 18275u, 351u, 7268u, 31319u, 30119u, 7600u, 2905u, 13826u, 11343u, 13053u, 15583u,
    30055u, 31093u, 5067u, 761u, 9685u, 11070u, 21369u, 27155u, 3663u, 26542u, 20169u, 12161u,
    15411u, 30401u, 7580u, 31784u, 8985u, 29367u, 20989u, 14203u, 29694u, 21167u, 10337u, 1706u,
    28578u, 887u, 3373u, 19477u, 14382u, 675u, 7033u, 15111u, 26138u, 12252u, 30996u, 21409u,
    25678u, 18555u, 13256u, 23316u, 22407u, 16727u, 991u, 9236u, 5373u, 29402u, 6117u, 15241u,
    27715u, 19291u, 19888u, 19847u
};

static u32 g_gl_seed;

static void sh_u_srand(u32 seed)
{
    g_gl_seed = g_seed_table[seed & 0xFF];
}

static u32 sh_u_random(void)
{
    g_gl_seed *= 69069u;
    g_gl_seed += g_seed_table[g_gl_seed & 0xFF] + 1u;
    return g_gl_seed & 0xFFFFFFFu;
}

static u32 sh_float_bits(f32 v)
{
    union { f32 f; u32 u; } c;
    c.f = v;
    return c.u;
}

f32 sh_shared_random_float(u32 seed, f32 low, f32 high)
{
    u32 range = (u32)(high - low);

    sh_u_srand(seed + sh_float_bits(low) + sh_float_bits(high));

    sh_u_random();
    sh_u_random();

    if (range)
    {
        i32 tensixrand = sh_u_random() & 0xFFFF;
        f32 offset = (f32)tensixrand / 65536.0f;
        return low + offset * (f32)range;
    }

    return low;
}

void sh_spread_compute(i32 seed, f32 spread, vec2_t *out)
{
    f32 x = sh_shared_random_float((u32)seed, -0.5f, 0.5f)
          + sh_shared_random_float((u32)seed + 1u, -0.5f, 0.5f);
    f32 y = sh_shared_random_float((u32)seed + 2u, -0.5f, 0.5f)
          + sh_shared_random_float((u32)seed + 3u, -0.5f, 0.5f);

    out->x = x * spread;
    out->y = y * spread;
}

const sh_recoil_t *sh_recoil_for(i32 weapon_id);

static sh_accuracy_t g_accuracy;
static u32 g_server_seed;

void sh_spread_feed_seed(u32 seed)
{
    g_server_seed = seed;
}

u32 sh_spread_seed(void)
{
    return g_server_seed;
}

static const f32 g_wpn_accuracy_init[] =
{
    0.0f, 0.9f, 0.9f, 0.0f, 0.9f, 0.9f, 0.9f,
    0.0f, 0.0f, 0.9f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.9f, 0.9f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.9f, 0.9f,
    0.0f, 0.0f, 0.0f, 0.0f
};

void sh_spread_reset(void)
{
    g_accuracy.accuracy = 0.0f;
    g_accuracy.shots = 0;
    g_accuracy.weapon_id = 0;
    g_accuracy.last_shot = 0.0f;
}

f32 sh_spread_accuracy(i32 weapon_id)
{
    if (weapon_id < 0 || weapon_id > 30)
        return 0.0f;

    return g_wpn_accuracy_init[weapon_id];
}

static void sh_accuracy_update(i32 weapon_id)
{
    f32 now = g_pmove ? g_pmove->time : 0.0f;

    if (g_accuracy.weapon_id != weapon_id)
    {
        g_accuracy.weapon_id = weapon_id;
        g_accuracy.accuracy = sh_spread_accuracy(weapon_id);
        g_accuracy.shots = 0;
    }

    f32 cycle = 0.0f;
    const sh_recoil_t *rec = sh_recoil_for(weapon_id);

    if (rec && rec->factor > 0)
        cycle = (f32)rec->factor * 0.01f;

    if (now - g_accuracy.last_shot > cycle + 0.25f)
        g_accuracy.shots = 0;

    ++g_accuracy.shots;

    f32 s = (f32)g_accuracy.shots;
    f32 acc = (s * s * s) / 200.0f + 0.35f;

    if (acc > 1.25f)
        acc = 1.25f;

    g_accuracy.accuracy = acc;
    g_accuracy.last_shot = now;
}

void sh_spread_angle(i32 weapon_id, i32 seed, f32 *out_x, f32 *out_y)
{
    if (!out_x || !out_y)
        return;

    sh_accuracy_update(weapon_id);

    f32 acc = g_accuracy.accuracy;
    f32 spread = 0.0f;

    qbool onground = 1;
    f32 speed2d = 0.0f;
    qbool ducking = 0;

    if (g_pmove)
    {
        onground = (g_pmove->onground != -1) ? 1 : 0;
        speed2d = sh_sqrt(g_pmove->velocity.x * g_pmove->velocity.x
                        + g_pmove->velocity.y * g_pmove->velocity.y);
        ducking = (g_pmove->usehull == 1) ? 1 : 0;
    }

    switch (weapon_id)
    {
    case 28:
        if (!onground)
            spread = 0.04f + 0.4f * acc;
        else if (speed2d > 140.0f)
            spread = 0.04f + 0.07f * acc;
        else if (ducking)
            spread = 0.0275f * acc;
        else
            spread = 0.0275f * acc;
        break;

    case 27:
        if (!onground)
            spread = 0.035f + 0.45f * acc;
        else if (speed2d > 140.0f)
            spread = 0.035f + 0.075f * acc;
        else
            spread = 0.02f * acc;
        break;

    case 22:
        if (!onground)
            spread = 0.04f + 0.4f * acc;
        else if (speed2d > 140.0f)
            spread = 0.04f + 0.07f * acc;
        else
            spread = 0.025f * acc;
        break;

    case 16:
        if (!onground)
            spread = 0.055f + 0.5f * acc;
        else if (speed2d > 140.0f)
            spread = 0.055f + 0.1f * acc;
        else
            spread = 0.035f * acc;
        break;

    case 17:
    case 2:
        if (!onground)
            spread = 0.08f + 0.2f * acc;
        else if (speed2d > 140.0f)
            spread = 0.08f + 0.04f * acc;
        else
            spread = 0.04f * acc;
        break;

    case 26:
        if (!onground)
            spread = 0.06f + 0.35f * acc;
        else if (speed2d > 140.0f)
            spread = 0.06f + 0.06f * acc;
        else
            spread = 0.03f * acc;
        break;

    default:
        spread = 0.03f * acc;
        break;
    }

    if (seed == 0)
        seed = (i32)g_server_seed;

    vec2_t sp;
    sh_spread_compute(seed, spread, &sp);

    *out_x = sp.x;
    *out_y = sp.y;
}

void sh_spread_trace_dir(const vec3_t *forward, const vec3_t *right,
                         const vec3_t *up, f32 sx, f32 sy, vec3_t *out)
{
    if (!forward || !right || !up || !out)
        return;

    out->x = forward->x + sx * right->x + sy * up->x;
    out->y = forward->y + sx * right->y + sy * up->y;
    out->z = forward->z + sx * right->z + sy * up->z;
}

void sh_spread_compensate(vec3_t *viewangles)
{
    if (!viewangles)
        return;

    i32 wid = sh_local_weapon_id();

    if (g_pmove)
    {
        viewangles->x -= g_pmove->punchangle.x;
        viewangles->y -= g_pmove->punchangle.y;

        if (viewangles->x > 89.0f)
            viewangles->x = 89.0f;
        if (viewangles->x < -89.0f)
            viewangles->x = -89.0f;

        while (viewangles->y > 180.0f)
            viewangles->y -= 360.0f;
        while (viewangles->y < -180.0f)
            viewangles->y += 360.0f;
    }

    f32 sx = 0.0f, sy = 0.0f;

    sh_spread_angle(wid, 0, &sx, &sy);

    if (sx == 0.0f && sy == 0.0f)
        return;

    f32 pitch_off = sy * 90.0f;
    f32 yaw_off = sx * 90.0f;

    viewangles->x -= pitch_off;
    viewangles->y -= yaw_off;

    if (viewangles->x > 89.0f)
        viewangles->x = 89.0f;
    if (viewangles->x < -89.0f)
        viewangles->x = -89.0f;

    while (viewangles->y > 180.0f)
        viewangles->y -= 360.0f;
    while (viewangles->y < -180.0f)
        viewangles->y += 360.0f;
}

typedef struct sh_recoil_set_s
{
    i32 weapon_id;
    sh_recoil_t st[4];
} sh_recoil_set_t;

static const sh_recoil_set_t g_recoil_tbl[] =
{
    { 28, { { 28, 1.5f, 0.45f, 0.225f, 0.05f, 6.5f, 2.5f, 7 },
            { 28, 2.0f, 1.0f, 0.5f, 0.35f, 9.0f, 6.0f, 5 },
            { 28, 0.9f, 0.35f, 0.15f, 0.025f, 5.5f, 1.5f, 9 },
            { 28, 1.0f, 0.375f, 0.175f, 0.0375f, 5.75f, 1.75f, 8 } } },

    { 22, { { 22, 1.0f, 0.45f, 0.28f, 0.045f, 3.75f, 3.0f, 7 },
            { 22, 1.2f, 0.5f, 0.23f, 0.15f, 5.5f, 3.5f, 6 },
            { 22, 0.6f, 0.3f, 0.2f, 0.0125f, 3.25f, 2.0f, 7 },
            { 22, 0.65f, 0.35f, 0.25f, 0.015f, 3.5f, 2.25f, 7 } } },

    { 19, { { 19, 0.9f, 0.475f, 0.35f, 0.0425f, 5.0f, 3.0f, 6 },
            { 19, 1.1f, 0.55f, 0.3f, 0.14f, 5.5f, 3.5f, 6 },
            { 19, 0.5f, 0.275f, 0.2f, 0.03f, 3.0f, 2.0f, 10 },
            { 19, 0.55f, 0.3f, 0.225f, 0.033f, 3.25f, 2.25f, 10 } } },

    { 8,  { { 8, 0.625f, 0.375f, 0.25f, 0.0125f, 3.5f, 2.25f, 8 },
            { 8, 0.75f, 0.45f, 0.25f, 0.13f, 5.0f, 3.25f, 7 },
            { 8, 0.55f, 0.3f, 0.2f, 0.0125f, 3.0f, 2.0f, 9 },
            { 8, 0.575f, 0.325f, 0.225f, 0.015f, 3.25f, 2.125f, 9 } } },

    { 15, { { 15, 0.625f, 0.375f, 0.25f, 0.0125f, 3.5f, 2.25f, 8 },
            { 15, 0.75f, 0.45f, 0.25f, 0.13f, 5.0f, 3.25f, 7 },
            { 15, 0.55f, 0.3f, 0.2f, 0.0125f, 3.0f, 2.0f, 9 },
            { 15, 0.575f, 0.325f, 0.225f, 0.015f, 3.25f, 2.125f, 9 } } },

    { 27, { { 27, 0.625f, 0.375f, 0.25f, 0.0125f, 4.0f, 2.25f, 9 },
            { 27, 0.775f, 0.45f, 0.25f, 0.13f, 5.5f, 3.25f, 8 },
            { 27, 0.55f, 0.3f, 0.2f, 0.0125f, 3.0f, 2.0f, 10 },
            { 27, 0.575f, 0.325f, 0.225f, 0.015f, 3.25f, 2.125f, 10 } } },

    { 14, { { 14, 0.65f, 0.35f, 0.25f, 0.015f, 3.5f, 2.25f, 7 },
            { 14, 0.8f, 0.45f, 0.25f, 0.13f, 5.0f, 3.25f, 7 },
            { 14, 0.55f, 0.3f, 0.2f, 0.0125f, 3.0f, 2.0f, 9 },
            { 14, 0.575f, 0.325f, 0.225f, 0.015f, 3.25f, 2.125f, 9 } } },

    { 16, { { 16, 0.625f, 0.375f, 0.25f, 0.0125f, 3.5f, 2.25f, 8 },
            { 16, 0.75f, 0.45f, 0.25f, 0.13f, 5.0f, 3.25f, 7 },
            { 16, 0.55f, 0.3f, 0.2f, 0.0125f, 3.0f, 2.0f, 9 },
            { 16, 0.575f, 0.325f, 0.225f, 0.015f, 3.25f, 2.125f, 9 } } },

    { 30, { { 30, 0.3f, 0.225f, 0.125f, 0.02f, 3.25f, 1.25f, 8 },
            { 30, 0.4f, 0.3f, 0.15f, 0.12f, 4.5f, 2.0f, 7 },
            { 30, 0.225f, 0.15f, 0.1f, 0.015f, 2.25f, 1.0f, 10 },
            { 30, 0.25f, 0.175f, 0.125f, 0.02f, 2.5f, 1.125f, 10 } } },

    { 12, { { 12, 0.275f, 0.2f, 0.15f, 0.0225f, 2.5f, 1.5f, 10 },
            { 12, 0.4f, 0.275f, 0.15f, 0.12f, 4.0f, 2.25f, 9 },
            { 12, 0.2f, 0.14f, 0.1f, 0.015f, 2.0f, 1.0f, 10 },
            { 12, 0.225f, 0.16f, 0.125f, 0.02f, 2.25f, 1.125f, 10 } } },

    { 7,  { { 7, 0.775f, 0.425f, 0.2f, 0.03f, 3.0f, 2.75f, 9 },
            { 7, 0.9f, 0.5f, 0.2f, 0.15f, 4.5f, 3.5f, 8 },
            { 7, 0.6f, 0.325f, 0.15f, 0.0225f, 2.75f, 2.0f, 10 },
            { 7, 0.625f, 0.35f, 0.175f, 0.025f, 3.0f, 2.125f, 10 } } },

    { 23, { { 23, 0.725f, 0.375f, 0.15f, 0.025f, 2.75f, 2.25f, 9 },
            { 23, 0.85f, 0.5f, 0.15f, 0.15f, 4.25f, 3.25f, 8 },
            { 23, 0.5f, 0.275f, 0.1f, 0.02f, 2.25f, 1.5f, 10 },
            { 23, 0.525f, 0.3f, 0.125f, 0.0225f, 2.5f, 1.625f, 10 } } },

    { 20, { { 20, 0.8f, 0.35f, 0.3f, 0.03f, 3.75f, 3.0f, 9 },
            { 20, 1.0f, 0.5f, 0.3f, 0.15f, 5.0f, 4.0f, 8 },
            { 20, 0.6f, 0.275f, 0.25f, 0.0225f, 3.0f, 2.25f, 10 },
            { 20, 0.625f, 0.3f, 0.275f, 0.025f, 3.25f, 2.375f, 10 } } }
};

static sh_recoil_t g_recoil_fallback = { 0, 0.65f, 0.375f, 0.25f, 0.015f, 3.5f, 2.25f, 8 };

static const sh_recoil_set_t *sh_recoil_set_for(i32 weapon_id)
{
    for (i32 i = 0; i < (i32)(sizeof(g_recoil_tbl) / sizeof(g_recoil_tbl[0])); ++i)
    {
        if (g_recoil_tbl[i].weapon_id == weapon_id)
            return &g_recoil_tbl[i];
    }

    return 0;
}

i32 sh_recoil_state_index(void)
{
    if (!g_pmove)
        return 3;

    f32 vx = g_pmove->velocity.x;
    f32 vy = g_pmove->velocity.y;

    f32 speed = sh_sqrt(vx * vx + vy * vy);

    if (speed > 0.0f)
        return 0;

    if (g_pmove->onground == -1)
        return 1;

    if (g_pmove->usehull == 1)
        return 2;

    return 3;
}

const sh_recoil_t *sh_recoil_for(i32 weapon_id)
{
    const sh_recoil_set_t *set = sh_recoil_set_for(weapon_id);

    if (!set)
        return &g_recoil_fallback;

    return &set->st[sh_recoil_state_index()];
}

void sh_recoil_apply(i32 shots, const sh_recoil_t *r, vec2_t *punch)
{
    if (!r || !punch)
        return;

    f32 up = r->up_base + r->up_mod * (f32)shots;
    f32 lateral = r->lateral_base + r->lateral_mod * (f32)shots;

    punch->x -= up;
    punch->y += lateral;
}

void sh_recoil_compensate(vec3_t *viewangles, i32 weapon_id, i32 shots)
{
    const sh_recoil_t *r = sh_recoil_for(weapon_id);
    vec2_t punch = { 0.0f, 0.0f };

    sh_recoil_apply(shots, r, &punch);

    viewangles->x -= punch.x;
    viewangles->y -= punch.y;
    viewangles->z = 0.0f;
}


static const sh_weapon_ballistic_t g_weapon_ballistic[] =
{
    {  0,   0.0f, 0.00f, 0 },
    {  1,  32.0f, 0.80f, 1 },
    {  2,  25.0f, 0.75f, 1 },
    {  3,  75.0f, 0.98f, 3 },
    {  4,  98.0f, 0.98f, 2 },
    {  5,   6.5f, 0.98f, 0 },
    {  6,   0.0f, 0.00f, 0 },
    {  7,  29.0f, 0.82f, 1 },
    {  8,  32.0f, 0.96f, 2 },
    {  9,   0.0f, 0.00f, 0 },
    { 10,  36.0f, 0.75f, 1 },
    { 11,  20.0f, 0.885f, 1 },
    { 12,  30.0f, 0.82f, 1 },
    { 13,  70.0f, 0.98f, 2 },
    { 14,  30.0f, 0.98f, 2 },
    { 15,  30.0f, 0.96f, 2 },
    { 16,  34.0f, 0.79f, 1 },
    { 17,  25.0f, 0.75f, 1 },
    { 18, 115.0f, 0.99f, 3 },
    { 19,  26.0f, 0.84f, 1 },
    { 20,  32.0f, 0.97f, 2 },
    { 21,   0.0f, 0.00f, 0 },
    { 22,  32.0f, 0.97f, 2 },
    { 23,  20.0f, 0.85f, 1 },
    { 24,  80.0f, 0.98f, 3 },
    { 25,   0.0f, 0.00f, 0 },
    { 26,  54.0f, 0.81f, 2 },
    { 27,  33.0f, 0.955f, 2 },
    { 28,  36.0f, 0.98f, 2 },
    { 29,  15.0f, 0.90f, 0 },
    { 30,  21.0f, 0.885f, 1 }
};

const sh_weapon_ballistic_t *sh_weapon_ballistic_for(i32 weapon_id)
{
    if (weapon_id < 0 || weapon_id > 30)
        return 0;

    return &g_weapon_ballistic[weapon_id];
}

f32 sh_penetration_modifier(i32 char_texture)
{
    switch (char_texture)
    {
    case 'C':
        return 0.30f;
    case 'M':
        return 0.65f;
    case 'D':
        return 0.18f;
    case 'V':
        return 0.40f;
    case 'G':
        return 0.55f;
    case 'T':
        return 0.45f;
    case 'S':
        return 0.05f;
    case 'W':
        return 0.12f;
    case 'P':
        return 0.60f;
    case 'X':
        return 0.10f;
    case 'Y':
        return 0.05f;
    case 'F':
        return 0.08f;
    case 'N':
        return 0.25f;
    default:
        return 0.35f;
    }
}

f32 sh_penetration_damage_modifier(i32 char_texture)
{
    switch (char_texture)
    {
    case 'C':
        return 0.85f;
    case 'M':
        return 0.60f;
    case 'D':
        return 0.95f;
    case 'V':
        return 0.80f;
    case 'G':
        return 0.75f;
    case 'T':
        return 0.80f;
    case 'S':
        return 0.99f;
    case 'W':
        return 0.97f;
    case 'P':
        return 0.70f;
    case 'X':
        return 0.98f;
    case 'Y':
        return 0.99f;
    case 'F':
        return 0.99f;
    case 'N':
        return 0.90f;
    default:
        return 0.90f;
    }
}
