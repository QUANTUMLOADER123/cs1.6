#pragma once

#include "sh_types.h"

#define sh_grenade_gravity 800.0f
#define sh_grenade_max_speed 750.0f
#define sh_grenade_max_points 96

typedef struct sh_grenade_path_s
{
    vec3_t points[sh_grenade_max_points];
    i32 count;
    f32 launch_speed;
    vec3_t impact;
    qbool hit;
} sh_grenade_path_t;

f32  sh_grenade_launch_speed(f32 pitch);
f32  sh_grenade_optimal_angle(void);
f32  sh_grenade_range(f32 theta_up, f32 start_z);
void sh_grenade_predict(const vec3_t *origin, const vec3_t *angles,
                        const vec3_t *player_velocity, sh_grenade_path_t *out);
void sh_grenade_damage_radius(const vec3_t *center, const vec3_t *victim,
                              f32 base_damage, f32 radius, f32 *out);
