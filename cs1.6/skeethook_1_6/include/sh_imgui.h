#pragma once

#include "sh_types.h"

/* Обёртка над Dear ImGui. Меню больше не самописное: все виджеты,
   скролл, перетаскивание окна и ввод даёт ImGui. */

qbool sh_imgui_init(void *game_window);
void sh_imgui_shutdown(void);
qbool sh_imgui_ready(void);

/* кадр: begin -> виджеты -> end */
void sh_imgui_begin(i32 width, i32 height, qbool menu_visible);
void sh_imgui_end(void);

void *sh_imgui_font(void);

/* пересобрать атлас под масштаб (вызывается при смене разрешения) */
void sh_imgui_scale_font(f32 scale);

/* реальная высота кадра в пикселях (GL_VIEWPORT), не из движка */
f32 sh_imgui_frame_height(void);

/* масштаб интерфейса: один для окна, отступов и шрифта */
f32 sh_imgui_ui_scale(void);
