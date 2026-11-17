#pragma once

#include "sh_types.h"

typedef struct sh_pattern_s
{
    const char *name;
    const u8 *bytes;
    const char *mask;
    i32 len;
} sh_pattern_t;

qbool sh_str_eq(const char *a, const char *b);
void *sh_module_base(const char *module);
u32 sh_module_size(const char *module);
void *sh_find_pattern(void *base, u32 size, const sh_pattern_t *pattern);
void *sh_find_pattern_in(const char *module, const sh_pattern_t *pattern);
void *sh_pattern_call_target(void *at);
void *sh_pattern_imm32(void *at);
i32 sh_pattern_rel32(void *at);
