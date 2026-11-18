#pragma once

#include "sh_types.h"

#define sh_skybox_count 12
#define sh_skybox_max 48

const char *sh_skybox_name(i32 index);
i32 sh_skybox_custom_count(void);
void sh_skybox_scan(void);
qbool sh_skybox_fast_init(void);
qbool sh_skybox_fast_ready(void);
qbool sh_skybox_fast_apply(const char *name);
void sh_skybox_apply(i32 index);
void sh_skybox_restore(void);
void sh_skybox_tick(void);
i32  sh_skybox_current(void);
