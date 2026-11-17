# skeethook_1_6 v1.0.0

SDK + чит для CS 1.6. Все адреса — по сигнатурам, без хардкода.

**DLL:** `skeethook_1_6.dll` — 361 KB (369 152 байта), 32-bit, PE32
**SHA256:** `045f283273b1dc22b28ef7d30820b17f874001273f19fc55f13e6b82b312c3b6`

## Установка

1. Инжектить `skeethook_1_6.dll` в `hl.exe` (32-bit), OpenGL-режим.
2. `INSERT` — меню. Конфиг автосохраняется при закрытии.

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
| add_entity | `0x01945F10` | chams / glow / no-flash |
| process_state | `0x01946010` | |
| txfer_overrides | `0x01945FC0` | |

## Хуки (9 через MinHook + 1 в интерфейсе)

| функция | что даёт |
|---|---|
| `CL_CreateMove` | ввод: антиаим, автострейфер, bhop, hideshot |
| `HUD_Redraw` | меню, ESP, индикаторы, туман/скайбокс |
| `HUD_PlayerMove` | автострейфер, quick stop, edge jump, лестницы |
| `HUD_AddEntity` | фильтр сущностей по имени модели (`no_smoke`), chams |
| `HUD_TxferLocalOverrides` | `fake_duck` через `origin`, `iuser1..4` |
| `HUD_TxferPredictionData` | `weapon_data_t[64]`: патроны, таймеры |
| `CL_CameraOffset` | thirdperson: `ofs[2] = dist` |
| `CL_IsThirdPerson` | включение вида от третьего лица |
| `HUD_GetStudioModelInterface` | `engine_studio_api_t` + подмена `StudioDrawPlayer` |
| `StudioDrawPlayer` (интерфейс) | **кости игроков**: 20 точек, поиск по имени |

## Структуры

| структура | размер | как подтверждено |
|---|---:|---|
| `sh_engfuncs_t` | `0x21C` | `rep movs ecx=0x87` в `Initialize` + typedef-assert |
| `sh_playermove_t` | `0x4F6BC` | 600 physent + 64 moveent + 600 visent + 600 pmtrace |
| `sh_physent_t` | `0xE0` | код `i*7<<5` |
| `sh_usercmd_t` | `0x34` | `rep stos ecx=0xD` |
| `sh_cl_entity_t` | `2996` | RTTI + sizeof-assert |
| `sh_clientdata_t` | `476` | из `cl_dll.h` |
| `sh_entity_state_t` | `340` | из `entity_state.h` |

## Фичи

**Ragebot** — enabled, silent, auto fire, auto wall, penetration,
double tap, auto stop, visible check, smoke check, hitbox (6), priority (4),
fov, smooth, min damage.

**Antiaim** — pitch (7 режимов: down/up/fake down/fake up/spin/random/off),
yaw (11: backwards/static L/R/left/right/spin/jitter/random/fake sideways),
desync + amount, roll, fake lag + ticks, yaw base, jitter, fake duck,
micro move + speed, movement fix.

**Visuals** — ESP (box 2 стиля, name, health bar, armor, weapon,
weapon icon, ammo, distance, skeleton, head dot, snaplines, team check,
visible only, show local, text scale, font), chams + team colors,
glow + amount, tracers, impacts + звук, thirdperson + dist,
nightmode (World | Sky | Lighting), skybox changer (12),
flat textures, hitmarker, hit sound, radar, spectator list,
grenade path, fire indicator, crosshair, hideshot, no flash, no smoke.

**Misc / Movement** — bhop, auto strafe, quick stop, super toss,
edge jump, fast ladder, auto pistol, fast reload, no recoil, no spread,
no scope, knife bot, fall damage warn, indicators.

## Честные границы

| фича | почему так |
|---|---|
| double tap | нет tickbase как в CS:GO; досыл `IN_ATTACK` в окне 1.5 frametime |
| desync | хитбокс в GoldSrc — коробка ±16, поворот модели его не двигает |
| roll | `viewangles.z` виден в демках/HLTV, камера не крутится |
| no spread | серверный разброс не отменить, компенсируется только крен |
| no fall damage | урон на сервере из `-pev->velocity.z`; есть только предупреждение |
| скелетон | `studiomodel` недоступен, рисуется 2D-проекция по боксу |

## Соответствие реверсу

Механики взяты из `cs1.6/reverse/`:

- air-strafe: cap 30 из `PM_AirAccelerate`, `omega = 90 − 180/(w/30+1)` → `sh_autostrafe`
- система координат: `right = (sin yaw, −cos yaw)` оттуда же → знак `sidemove`
- bhop: снятие `IN_JUMP` по `oldbuttons` → `sh_bhop`
- quick stop: `drop = friction * max(speed, 75) * frametime` из `PM_Friction`
- attack creep: `GetNextAttackDelay` (`- flCreep`) → допуск 0.5 frametime
- fake lag: `next_send_time` @ `0x2D5DA48`, clamp `cl_cmdrate` 100
- magic pitch 1620/−1619: ломает хитбокс, подтверждено расчётом
- гранаты: `flVel = (90−pitch)*6`, кап 750, g = 800, оптимум 42°
- нож: `STAB_DISTANCE 32`, `SWING_DISTANCE 48`, `BACKSTAB 3x`
- пробитие: 21/15/30/39/35/45/30/25 по материалам
- 59 opcodes `svc_*` из `hw.dll` — основа парсера демок
- `delta_description_t` обязателен до чтения потока сущностей

Подробности — `cs1.6/reverse/` (21 документ) и `16-feature-map.md`.
