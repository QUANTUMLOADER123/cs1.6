# skeethook_1_6 v1.0.0

SDK + чит для CS 1.6. Все адреса — по сигнатурам, без хардкода.

**DLL:** `skeethook_1_6.dll` — 321 KB, 32-bit, PE32
**SHA256:** `ae4e57262aff56f4f48f8abe3dd4a4433cfcdbc82937db5b12c6826d4975136f`

## Установка

1. Инжектить `skeethook_1_6.dll` в `hl.exe` (32-bit), OpenGL-режим.
2. `INSERT` — меню.

## Экспорты

| функция | назначение |
|---|---|
| `sh_get_name` | `"skeethook_1_6"` |
| `sh_get_version` | `"1.0.0"` |
| `sh_get_config` | указатель на `sh_config` (прямое управление) |
| `sh_is_attached` | подключился ли к `client.dll` |

## Сигнатуры (проверены на уникальность)

| имя | адрес | даёт |
|---|---|---|
| initialize | `0x01944710` | `gEngfuncs` = `0x01A25CC0` |
| player_move | `0x01963050` | `g_pmove` = `0x01A36D84` |
| player_move_init | `0x01963120` | инициализация |
| create_move | `0x01958CC0` | хук ввода |
| hud_redraw | `0x01944A60` | хук рендера |

## Структуры

| структура | размер | как подтверждено |
|---|---:|---|
| `sh_engfuncs_t` | `0x21C` | `rep movs ecx=0x87` в `Initialize` + typedef-assert |
| `sh_playermove_t` | `1508` | оффсеты полей из `HUD_PlayerMove` |
| `sh_physent_t` | `0xE0` | код `i*7<<5` |
| `sh_usercmd_t` | `0x34` | `rep stos ecx=0xD` |

## Фичи

**Aimbot** — fov, smooth, hitbox, autowall, silent, auto fire, visible check.
**Antiaim** — pitch (down/up/fake/random), yaw (backward/spin/jitter/LBY), desync, fake duck, micro move.
**Misc** — bhop, auto strafe, edge jump, fast ladder, no fall damage, auto pistol, fast reload, no recoil/spread/flash/smoke/scope.
**ESP** — боксы (2 стиля), имя, HP, броня, оружие, дистанция, скелет, head dot, snaplines.

## Соответствие реверсу

Механики взяты из `cs1.6/reverse/`:

- air-strafe: cap 30 из `PM_AirAccelerate` → `sh_auto_strafe`
- bhop: фактор 1.2 / штраф 0.8 из `PM_PreventMegaBunnyJumping` → `sh_bhop`
- edge friction 2.0 из `PM_Friction` → `sh_edge_jump`
- MAX_CLIMB_SPEED 200 → `sh_fast_ladder`
- range_modifier оружия → autowall в `sh_can_hit`
