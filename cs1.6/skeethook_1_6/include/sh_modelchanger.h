#pragma once

#include "sh_types.h"

const char *sh_model_changer_name(i32 index);
i32 sh_model_changer_count(void);
void sh_model_changer_tick(void);
i32 sh_model_changer_applied(void);
