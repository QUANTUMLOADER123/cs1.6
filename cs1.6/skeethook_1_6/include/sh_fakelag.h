#pragma once

#include "sh_types.h"
#include "sh_sdk.h"

#define sh_fakelag_max_choke 22

void sh_fakelag_run(sh_usercmd_t *cmd);
qbool sh_fakelag_should_choke(void);
void sh_fakelag_reset(void);
i32 sh_fakelag_choking(void);

extern f32 g_next_send_time;
void sh_fakelag_init(void);
