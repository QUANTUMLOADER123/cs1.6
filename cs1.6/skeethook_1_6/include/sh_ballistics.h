#pragma once

#include "sh_types.h"

typedef struct sh_recoil_s
{
    i32 weapon_id;
    f32 up_base;
    f32 lateral_base;
    f32 up_mod;
    f32 lateral_mod;
    f32 up_variance;
    f32 lateral_variance;
    i32 factor;
} sh_recoil_t;

f32  sh_shared_random_float(u32 seed, f32 low, f32 high);
void sh_spread_compute(i32 seed, f32 spread, vec2_t *out);

typedef struct sh_accuracy_s
{
    f32 accuracy;
    i32 shots;
    i32 weapon_id;
    f32 last_shot;
} sh_accuracy_t;

void sh_spread_angle(i32 weapon_id, i32 seed, f32 *out_x, f32 *out_y);
void sh_spread_trace_dir(const vec3_t *forward, const vec3_t *right,
                         const vec3_t *up, f32 sx, f32 sy, vec3_t *out);
f32  sh_spread_accuracy(i32 weapon_id);
void sh_spread_reset(void);
void sh_spread_feed_seed(u32 seed);
u32  sh_spread_seed(void);
void sh_spread_compensate(vec3_t *viewangles);

const sh_recoil_t *sh_recoil_for(i32 weapon_id);
void sh_recoil_apply(i32 shots, const sh_recoil_t *r, vec2_t *punch);
void sh_recoil_compensate(vec3_t *viewangles, i32 weapon_id, i32 shots);

f32 sh_penetration_modifier(i32 char_texture);
f32 sh_penetration_damage_modifier(i32 char_texture);
