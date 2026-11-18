#pragma once

#include "sh_types.h"

void sh_worldfx_tick(void);
void sh_worldfx_hit_sound(void);
void sh_fog_apply(void);
qbool sh_fog_enabled(void);
f32 sh_fog_density(void);
void sh_worldfx_hit_sound_damage(i32 dmg);
