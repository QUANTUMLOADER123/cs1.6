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
