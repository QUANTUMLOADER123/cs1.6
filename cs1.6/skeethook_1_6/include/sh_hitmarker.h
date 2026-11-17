#pragma once

#include "sh_types.h"

void sh_hitmarker_on_hit(qbool headshot, i32 damage);
void sh_hitmarker_tick(f32 time);
void sh_hitmarker_draw(void);
i32  sh_hitmarker_count(void);
