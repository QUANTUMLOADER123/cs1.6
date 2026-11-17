# skeethook_1_6

SDK + чит для Counter-Strike 1.6. Собран на восстановленных из реверса
структурах и сигнатурах. **Ни одного хардкод-оффсета** — все адреса ищутся
сканированием по паттернам и проверены на реальном `client.dll`.

```
skeethook_1_6.dll   321 KB    32-bit, инжектится в hl.exe / cstrike
```

## Сборка

```bash
./build.sh
```

Нужен `zig` (или `pip install ziglang`). MinHook уже в `ext/minhook`.
Результат: `build/skeethook_1_6.dll`.

## Что внутри

| слой | файлы | содержание |
|---|---|---|
| ядро | `src/core/sh_scanner.cpp` | сканер паттернов с масками |
| | `src/core/sh_hooks.cpp` | хуки `CL_CreateMove`, `HUD_Redraw`, `HUD_PlayerMove` |
| | `src/core/sh_config.cpp` | конфиг, автосохранение |
| | `src/core/sh_math.cpp` | векторы, углы, `world_to_screen` |
| SDK | `src/sdk/sh_patterns.cpp` | резолв `gEngfuncs`, `g_pmove` по сигнатурам |
| рендер | `src/render/sh_render.cpp` | примитивы через OpenGL |
| | `src/render/sh_font.cpp` | свой растровый шрифт 8x10, атлас 16x6 |
| фичи | `src/features/*` | aimbot, antiaim, misc, esp, menu |

## API

```c
#include "sh_sdk.h"

// указатели, полученные по сигнатурам
sh_engfuncs_t   *g_engfuncs;    // 540 Б, 135 функций движка
sh_playermove_t *g_pmove;       // физика, 0x4F6BC Б
i32              g_local_player;

// проверка
if (sh_sdk_ready()) { ... }

// игроки
sh_players_collect();
for (i32 i = 0; i < sh_players_count(); ++i) {
    sh_player_t *p = sh_player_get(i);
    // p->name, p->health, p->armor, p->weapon_name, p->distance, p->visible
}

// рендер
sh_render_begin(w, h);
sh_draw_rect(x, y, w, h, col);
sh_draw_rect_outline(x, y, w, h, 1.0f, col);
sh_text(x, y, col, "%s", "hi");
sh_render_end();
```

## Проверенные структуры

Раскладка сверена с бинарником, размеры подтверждены компилятором
(`static_assert` через typedef-массивы):

| структура | размер | подтверждение |
|---|---:|---|
| `sh_engfuncs_t` | `0x21C` | `rep movs ecx=0x87` в `Initialize` |
| `sh_playermove_t` | `0x4F6BC` | 600 physent + 64 moveent + 600 visent + 600 pmtrace |
| `sh_physent_t` | `0xE0` | код `i*7<<5` = `i*224` |
| `sh_usercmd_t` | `0x34` | `rep stos ecx=0xD` в `CL_CreateMove` |

## Сигнатуры

Все пять проверены на уникальность в `.text` реального `client.dll`:

| имя | адрес | что даёт |
|---|---|---|
| `initialize` | `0x01944710` | `gEngfuncs` через `BF imm32` |
| `player_move` | `0x01963050` | `g_pmove` через `A3 imm32` |
| `player_move_init` | `0x01963120` | инициализация физики |
| `create_move` | `0x01958CC0` | точка хука ввода |
| `hud_redraw` | `0x01944A60` | точка хука рендера |

Проверка прямо из кода:

```bash
python3 tools/analysis/sig_gen.py client.dll --min 22 --at 0x1944710 0x1963050
```

## Фичи

### Aimbot
- выбор цели по FOV, приоритету, дистанции
- хитбоксы: голова, шея, грудь, живот, таз, ноги (смещения в `g_hitbox_offsets`)
- **auto wall** — проверка пробития с учётом `range_modifier`
- **visible check** — трассировка `PM_TraceLine`
- **silent aim** — подмена углов в `usercmd_t`
- плавность (smooth), автоогонь, клавиша активации

### Antiaim
- pitch: down / up / fake / random
- yaw: backward / spin / jitter / fake side / LBY break
- **desync** — рассинхрон моделей
- **fake duck** — присед через тики
- **micro move** — движение по квадрату
- вращение (spin) с настраиваемой скоростью

### Misc
- **bunny hop** — на основе `PM_PreventMegaBunnyJumping` (фактор 1.2)
- **auto strafe** — air-strafe с учётом cap 30 у `PM_AirAccelerate`
- **edge jump** — прыжок с края по трассировке
- **fast ladder** — 200 ед/с (`MAX_CLIMB_SPEED`)
- no fall damage, auto pistol, fast reload
- no recoil, no spread, no scope, no flash, no smoke

### ESP
- боксы (2 стиля: полный / угловой)
- имя, здоровье (полоса + число), броня
- оружие, дистанция
- скелет (11 точек, 10 костей)
- head dot, snaplines
- цвета видимый/скрытый, фильтр по команде

## Меню

`INSERT` — открыть. Табы: rage, antiaim, visuals, misc, settings.
Конфиг сохраняется при закрытии.

## Архитектурные решения

1. **Паттерны вместо оффсетов.** Каждый адрес — результат сканирования
   маски. `sig_gen.py` доказывает уникальность.
2. **Свой рендер.** Свой шрифт 8x10 в атласе, примитивы через
   `opengl32.dll` напрямую. Зависимость только от GL, не от ImGui.
3. **Вызовы через указатели.** `g_engfuncs` и `g_pmove` — это указатели,
   которые движок сам заполняет. Чит не «ломает» их, а читает.
4. **Точки хука.** `CL_CreateMove` — там же, где работает игра; `HUD_Redraw` —
   после того, как движок нарисовал HUD.

## Ограничения

- **32-bit**. Инжектить в 32-битный процесс (hl.exe).
- Требуется OpenGL-режим, не D3D9 (`opengl32.dll`).
- Сигнатуры привязаны к версии клиента
  (`client.pdb` → `GoldSrc\cstrike\cl_dll\Release`). При другой сборке
  перегенерировать `sig_gen.py`.
- `g_pmove` валиден только между `HUD_PlayerMoveInit` и выгрузкой; до
  коннекта к серверу он нулевой — это обрабатывается в `sh_sdk_ready()`.

## Структура

```
include/          sh_types, sh_config, sh_sdk, sh_math, sh_scanner, sh_render, sh_font, sh_hooks, sh_features
src/core/         сканер, математика, конфиг, хуки, точка входа
src/sdk/          резолв по сигнатурам
src/render/       GL-примитивы, шрифт, атлас
src/features/     aimbot, antiaim, misc, esp, menu, players
ext/minhook/      MinHook (TsudaKageyu)
```
