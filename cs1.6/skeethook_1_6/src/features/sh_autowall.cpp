#include "../../include/sh_autowall.h"
#include "../../include/sh_ballistics.h"
#include "../../include/sh_math.h"
#include "../../include/sh_sdk.h"
#include "../../include/sh_features.h"

static f32 _pow(f32 b, f32 e)
{
    f32 r = 1.0f;
    for (i32 i = 0; i < (i32)e; ++i) r *= b;
    f32 frac = e - (f32)(i32)e;
    if (frac > 0.0f) r *= 1.0f + frac * (b - 1.0f);
    return r;
}

static vec3_t _sub(vec3_t a, vec3_t b) { vec3_t o; sh_sub(&o, &a, &b); return o; }
static vec3_t _add(vec3_t a, vec3_t b) { vec3_t o; sh_add(&o, &a, &b); return o; }
static vec3_t _scale(vec3_t v, f32 s) { vec3_t o; sh_scale(&o, &v, s); return o; }
static vec3_t _norm(vec3_t v) { sh_normalize(&v); return v; }

static const sh_bullet_type_t g_bullet_power[] =
{
    { 1,  21,  800 },
    { 2,  15,  500 },
    { 3,  30, 1000 },
    { 4,  39, 5000 },
    { 5,  35, 4000 },
    { 6,  45, 8000 },
    { 7,  30, 2000 },
    { 8,  25,  800 }
};

const sh_bullet_type_t *sh_bullet_type_for(i32 bullet_id)
{
    for (u32 i = 0; i < sh_countof(g_bullet_power); ++i)
    {
        if (g_bullet_power[i].id == bullet_id)
            return &g_bullet_power[i];
    }
    return 0;
}

i32 sh_hitgroup_multiplier(i32 hitgroup)
{
    switch (hitgroup)
    {
    case 1:
        return 400;
    case 3:
        return 125;
    case 6:
    case 7:
        return 75;
    case 0:
        return 0;
    default:
        return 100;
    }
}

f32 sh_damage_at_range(f32 base_damage, f32 range_modifier, f32 distance)
{
    return base_damage * _pow(range_modifier, distance / 500.0f);
}

qbool sh_autowall_trace(const sh_autowall_ctx_t *ctx, sh_autowall_result_t *out)
{
    if (!ctx || !out || !ctx->trace)
        return 0;

    const sh_bullet_type_t *bt = sh_bullet_type_for(ctx->bullet_id);
    if (!bt)
        return 0;

    out->damage = 0.0f;
    out->penetrated = 0;
    out->distance = 0.0f;

    vec3_t src = ctx->start;
    vec3_t delta = _sub(ctx->end, ctx->start);
    vec3_t dir = _norm(delta);
    f32 distance = sh_len3d(&delta);
    i32 penetration = ctx->penetration;
    f32 damage = ctx->base_damage;
    f32 power = (f32)bt->power;
    f32 traveled = 0.0f;

    if (distance <= 0.0f)
        return 0;

    for (i32 step = 0; step < 8 && penetration > 0; ++step)
    {
        vec3_t end = _add(src, _scale(dir, distance));
        sh_trace_t tr;

        if (!ctx->trace(ctx->user, &src, &end, &tr))
            break;

        if (tr.fraction >= 1.0f)
        {
            out->damage = damage;
            out->distance = traveled + distance;
            return 1;
        }

        f32 hit_distance = tr.fraction * distance;
        traveled += hit_distance;

        damage = sh_damage_at_range(damage, ctx->range_modifier, traveled);

        if (traveled > bt->distance)
        {
            out->damage = 0.0f;
            return 0;
        }

        if (tr.hitgroup > 0)
        {
            out->damage = damage * ((f32)sh_hitgroup_multiplier(tr.hitgroup) / 100.0f);
            out->distance = traveled;
            out->hitgroup = tr.hitgroup;
            return 1;
        }

        f32 tex_mod = sh_penetration_modifier(tr.char_texture);
        f32 dmg_mod = sh_penetration_damage_modifier(tr.char_texture);

        if (tex_mod >= 1.0f && !ctx->penetrate_world)
        {
            out->damage = 0.0f;
            return 0;
        }

        power *= tex_mod;

        if (power < 10.0f)
        {
            out->damage = 0.0f;
            return 0;
        }

        damage *= dmg_mod;
        power = 42.0f;

        src = _add(tr.end, _scale(dir, power));
        distance = (distance - hit_distance) * 0.75f;
        penetration--;
        out->penetrated = 1;

        if (distance <= 0.0f)
            break;
    }

    out->damage = damage;
    out->distance = traveled;
    return damage > 0.0f;
}

f32 sh_autowall_damage(const sh_autowall_ctx_t *ctx)
{
    sh_autowall_result_t r;
    if (!sh_autowall_trace(ctx, &r))
        return 0.0f;
    return r.damage;
}

i32 sh_autowall_walls(const vec3_t *start, const vec3_t *dir, i32 weapon_id)
{
    if (!start || !dir || !g_trace_fn)
        return 0;

    const sh_weapon_ballistic_t *wb = sh_weapon_ballistic_for(weapon_id);

    if (!wb || wb->damage <= 0.0f)
        return 0;

    i32 pen_left = wb->penetration;
    f32 damage = wb->damage;
    f32 traveled = 0.0f;

    vec3_t from = *start;
    i32 walls = 0;

    for (i32 step = 0; step < 8 && pen_left > 0; ++step)
    {
        vec3_t to;
        to.x = from.x + dir->x * 8192.0f;
        to.y = from.y + dir->y * 8192.0f;
        to.z = from.z + dir->z * 8192.0f;

        f32 frac = 1.0f;

        if (!sh_trace_fraction(&from, &to, &frac))
            break;

        if (frac >= 0.999f)
            break;

        if (frac <= 0.0f)
            break;

        f32 dist = frac * 8192.0f;
        traveled += dist;

        damage = damage * _pow(wb->range_modifier, dist / 500.0f);

        if (damage < 1.0f)
            break;

        vec3_t hit;
        hit.x = from.x + (to.x - from.x) * frac;
        hit.y = from.y + (to.y - from.y) * frac;
        hit.z = from.z + (to.z - from.z) * frac;

        i32 tex = sh_trace_material(&from, &hit);
        f32 mod = sh_penetration_modifier(tex);

        if (mod >= 0.6f && tex != 'G')
            break;

        damage *= sh_penetration_damage_modifier(tex);

        if (damage < 1.0f)
            break;

        ++walls;
        --pen_left;

        from.x = hit.x + dir->x * 16.0f;
        from.y = hit.y + dir->y * 16.0f;
        from.z = hit.z + dir->z * 16.0f;
    }

    (void)traveled;

    return walls;
}
