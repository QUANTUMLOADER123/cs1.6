#pragma once

#include "sh_types.h"

typedef struct sh_bullet_type_s
{
    i32 id;
    i32 power;
    i32 distance;
} sh_bullet_type_t;

typedef struct sh_trace_s
{
    f32 fraction;
    vec3_t end;
    vec3_t normal;
    i32 hitgroup;
    i32 char_texture;
    i32 entity;
} sh_trace_t;

typedef qbool (*sh_trace_fn)(void *user, const vec3_t *start, const vec3_t *end, sh_trace_t *out);

typedef struct sh_autowall_ctx_s
{
    vec3_t start;
    vec3_t end;
    i32 bullet_id;
    i32 penetration;
    f32 base_damage;
    f32 range_modifier;
    qbool penetrate_world;
    sh_trace_fn trace;
    void *user;
} sh_autowall_ctx_t;

typedef struct sh_autowall_result_s
{
    f32 damage;
    f32 distance;
    i32 hitgroup;
    qbool penetrated;
} sh_autowall_result_t;

const sh_bullet_type_t *sh_bullet_type_for(i32 bullet_id);
i32  sh_hitgroup_multiplier(i32 hitgroup);
f32  sh_damage_at_range(f32 base_damage, f32 range_modifier, f32 distance);
qbool sh_autowall_trace(const sh_autowall_ctx_t *ctx, sh_autowall_result_t *out);
f32  sh_autowall_damage(const sh_autowall_ctx_t *ctx);
i32  sh_autowall_walls(const vec3_t *start, const vec3_t *dir, i32 weapon_id);
