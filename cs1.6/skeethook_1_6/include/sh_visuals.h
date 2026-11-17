#pragma once

#include "sh_types.h"
#include "sh_sdk.h"

void sh_visuals_apply_nightmode(void);
void sh_visuals_restore(void);
void sh_visuals_thirdperson(sh_usercmd_t *cmd);
void sh_visuals_tracer_add(const vec3_t *from, const vec3_t *to);
void sh_visuals_impact_add(const vec3_t *pos, const vec3_t *normal);
void sh_visuals_effects_tick(f32 frametime);
void sh_visuals_effects_draw(void);

#define sh_max_tracers 64
#define sh_tracer_life 2.0f
