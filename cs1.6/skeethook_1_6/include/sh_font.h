#pragma once

#include "sh_types.h"

/* Шрифт рисуется GDI (Tahoma) в texture-атлас при инициализации,
   встроенная пиксельная таблица 8x10 больше не используется. */
#define sh_font_px_h 14

qbool sh_font_init(void);
void sh_font_shutdown(void);
void sh_font_bind(void);
f32 sh_font_width(const char *str, f32 scale);
void sh_font_draw(f32 x, f32 y, f32 scale, col4_t col, const char *str);
