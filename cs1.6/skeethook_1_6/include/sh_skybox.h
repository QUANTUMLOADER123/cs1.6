#pragma once

#include "sh_types.h"

#define sh_skybox_count 12

const char *sh_skybox_name(i32 index);
void sh_skybox_apply(i32 index);
void sh_skybox_restore(void);
void sh_skybox_tick(void);
i32  sh_skybox_current(void);
