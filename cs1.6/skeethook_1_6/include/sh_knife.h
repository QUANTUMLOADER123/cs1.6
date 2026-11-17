#pragma once

#include "sh_types.h"
#include "sh_sdk.h"

#define sh_knife_stab_damage 65.0f
#define sh_knife_swing_damage 15.0f
#define sh_knife_swing_fast 20.0f
#define sh_knife_stab_distance 32.0f
#define sh_knife_swing_distance 48.0f
#define sh_knife_backstab 3.0f

void sh_knife_run(sh_usercmd_t *cmd);
qbool sh_knife_should_stab(const vec3_t *target, const vec3_t *eye,
                           const vec3_t *view_angles, f32 distance);
