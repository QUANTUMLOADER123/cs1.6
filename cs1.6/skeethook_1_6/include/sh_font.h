#pragma once

#include "sh_types.h"

extern const u8 g_font_8x10[95][10];
extern const i32 g_font_8x10_w;
extern const i32 g_font_8x10_h;
extern const i32 g_font_8x10_first;
extern const i32 g_font_8x10_count;

qbool sh_font_init(void);
void sh_font_shutdown(void);
void sh_font_bind(void);
f32 sh_font_width(const char *str, f32 scale);
void sh_font_draw(f32 x, f32 y, f32 scale, col4_t col, const char *str);
