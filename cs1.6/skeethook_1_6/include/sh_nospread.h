#pragma once

#include "sh_types.h"

void sh_nospread_reset(void);
void sh_nospread_on_fire(i32 weapon_id);
f32  sh_nospread_accuracy(void);
f32  sh_nospread_spread(i32 weapon_id, qbool silenced, qbool in_burst,
                        i32 fov);
void sh_nospread_angles(i32 weapon_id, u32 seed, qbool silenced,
                        qbool in_burst, vec3_t *viewangles);
f32  sh_nospread_radius(i32 weapon_id);
