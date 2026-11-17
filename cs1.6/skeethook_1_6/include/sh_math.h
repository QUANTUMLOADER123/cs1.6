#pragma once

#include "sh_types.h"

#define sh_pi 3.14159265358979323846f
#define sh_rad2deg (180.0f / sh_pi)
#define sh_deg2rad (sh_pi / 180.0f)

f32 sh_deg(f32 rad);
f32 sh_rad(f32 deg);
f32 sh_clampf(f32 v, f32 lo, f32 hi);
i32 sh_clampi(i32 v, i32 lo, i32 hi);
f32 sh_len2d(const vec3_t *v);
f32 sh_len3d(const vec3_t *v);
f32 sh_dot(const vec3_t *a, const vec3_t *b);
void sh_sub(vec3_t *out, const vec3_t *a, const vec3_t *b);
void sh_add(vec3_t *out, const vec3_t *a, const vec3_t *b);
void sh_scale(vec3_t *out, const vec3_t *v, f32 s);
void sh_normalize(vec3_t *v);
void sh_cross(vec3_t *out, const vec3_t *a, const vec3_t *b);
void sh_angle_vectors(const vec3_t *angles, vec3_t *forward, vec3_t *right, vec3_t *up);
void sh_vectors_angle(const vec3_t *forward, vec3_t *angles);
f32 sh_angle_delta(f32 a, f32 b);
void sh_angle_normalize(vec3_t *angles);
f32 sh_normalize_yaw(f32 y);
qbool sh_world_to_screen(const vec3_t *origin, vec2_t *out);

f32 sh_sqrt(f32 v);
f32 sh_sin(f32 rad);
f32 sh_cos(f32 rad);
f32 sh_atan(f32 v);
f32 sh_atan2(f32 y, f32 x);
