#pragma once

#include "sh_types.h"

typedef struct sh_font_s sh_font_t;

void sh_render_init(void);
qbool sh_render_ready(void);
void sh_render_begin(i32 width, i32 height);
void sh_render_end(void);

void sh_draw_rect(f32 x, f32 y, f32 w, f32 h, col4_t col);
void sh_draw_rect_outline(f32 x, f32 y, f32 w, f32 h, f32 thick, col4_t col);
void sh_draw_rect_gradient(f32 x, f32 y, f32 w, f32 h, col4_t top, col4_t bottom);
void sh_draw_line(f32 x0, f32 y0, f32 x1, f32 y1, f32 thick, col4_t col);
void sh_draw_circle(f32 cx, f32 cy, f32 r, i32 seg, col4_t col);
void sh_draw_circle_outline(f32 cx, f32 cy, f32 r, i32 seg, f32 thick, col4_t col);
void sh_draw_triangle(vec2_t a, vec2_t b, vec2_t c, col4_t col);

void sh_text_push(void);
void sh_text_pop(void);
void sh_text_esp_begin(void);
void sh_text_esp_end(void);

void sh_text(f32 x, f32 y, col4_t col, const char *fmt, ...);
void sh_text_center(f32 cx, f32 y, col4_t col, const char *fmt, ...);
f32 sh_text_width(const char *str);
f32 sh_text_height(void);
void sh_text_shadow(f32 x, f32 y, col4_t col, const char *fmt, ...);

i32 sh_screen_width(void);
i32 sh_screen_height(void);
f32 sh_scale(void);
f32 sh_xres(f32 x);
f32 sh_yres(f32 y);
