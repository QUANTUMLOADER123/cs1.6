# skeethook_1_6 v1.0.0

SDK + чит для CS 1.6. Все адреса — по сигнатурам, без хардкода.

**DLL:** `skeethook_1_6.dll` — 435 KB (1155072 байт), 32-bit, PE32
**SHA256:** `7781ee9cc6669f90705290faadf0efc67888a28fd60c50de4a31e3876a374b0f`

## Установка

1. Инжектить `skeethook_1_6.dll` в `hl.exe` (32-bit), OpenGL-режим.
   Можно в меню или в игре — мод ждёт до 10 минут. Но `g_pmove`
   появляется только когда загружена карта, поэтому если инжектнули
   в главном меню, фичи включатся не раньше загрузки карты.
   Рядом с `hl.exe` создаётся `skeethook_trace.log` — там видно,
   докуда дошёл мод.
2. `INSERT` — меню. Конфиг автосохраняется при закрытии.

### Требования

| требование | почему |
|---|---|
| OpenGL-режим | ESP/меню рисуются через `glVertex2f`/`glBegin` из `opengl32.dll`; в D3D их нет, мод просто не нарисует ничего |
| Windows 10/11 | DLL слинкована с UCRT (`api-ms-win-crt-*`). На Win7 без обновления KB2999226 её не загрузит |
| 32-bit `hl.exe` | DLL собрана под `i386`, в 64-bit процесс не загрузится |
| выключать при выходе | при завершении процесса хуки не снимаются специально (см. ниже) |

### Если что-то пошло не так

| симптом | причина |
|---|---|
| ничего не происходит | смотрите `skeethook_trace.log`: если там «ждём player_move», нужна загруженная карта; если лог не создан — не нашёлся `client.dll` |
| краш сразу | не OpenGL-режим или 64-битный инжектор |
| не рисует меню, но остальное работает | `opengl32.dll` подгрузился после нас; перезапустить игру и инжектить в главном меню |
| зависание при выгрузке | `FreeLibrary` не используется, плагин живёт до выхода из игры |

## Экспорты

| функция | назначение |
|---|---|
| `sh_get_name` | `"skeethook_1_6"` |
| `sh_get_version` | `"1.0.0"` |
| `sh_get_config` | указатель на `sh_config` (прямое управление) |
| `sh_is_attached` | подключился ли к `client.dll` |
| `sh_test_crash_report` | проверить обработчик крашей без падения |

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
| chams_model | `0x01D869E0` | хук отрисовки модели, GL-проходы |
| chams_light | `0x01D83DE0` | хук освещения вершины, rim |
| chams_alpha | `0x01D88200` | `+34` → `float*` альфы |
| world_transform | `0x01D7E080` | `+0xC` → матрица вида |
| screen | `0x01D458F0` | `+2/+7/+0xD/+0x15` → размеры |
| viewport | `0x01D450F1` | `+2/+0xC` → viewport |
| load_skys | `0x01D4F9C2` | мгновенная смена неба |
| load_skys_name | `0x01D4FA51` | `+2` → буфер имени |

## Хуки (18 через MinHook)

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
double tap, auto stop (+ duck), auto scope, visible check, smoke check,
hitbox (6, влияет на точку прицела по высоте), priority (4),
fov, smooth, min damage.

**Antiaim** — pitch (7 режимов: down/up/fake down/fake up/spin/random/off),
yaw (14: backwards/static L/R/left/right/spin/jitter/random/fake sideways/
fake edge/at target/at target jitter),
desync + amount, roll, fake lag + ticks, yaw base, jitter, fake duck,
micro move + speed, movement fix.

**Visuals** — ESP (box 2 стиля, name, health bar, armor, weapon,
weapon icon, ammo, distance, skeleton, head dot, snaplines, team check,
visible only, show local, text scale, font), chams + team colors,
glow + amount, tracers, impacts + звук, thirdperson + dist,
nightmode (World | Sky | Lighting), skybox changer (сканирует gfx/env,
мгновенная смена без перезахода на карту), chams rim-подсветка,
fog (density + 3 режима near/far),
flat textures, hitmarker, hit sound, radar, spectator list,
grenade path, fire indicator, crosshair, hideshot, no flash, no smoke,
model changer (14 моделей), sound esp, fake markers, wallbang info,
position history, damage info (урон по цели с учётом хитбокса).

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
| fake edge | двигает yaw к ближайшей стене, но не телепортирует хитбокс |
| at target | угол на ближайшего врага по `origin`, а не по хитбоксу головы |
| wallbang info | считает проходимость по материалам, а не серверный урон |

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

## Если игра вылетела

Рядом с `hl.exe` появятся файлы, и тот же текст уже лежит в буфере
обмена — можно сразу нажать `Ctrl+V` в чат, ничего не искать.

| файл | что внутри |
|---|---|
| `skeethook_crash.log` | код исключения, адрес падения с именем модуля, регистры, 8 значений со стека, последний этап работы |
| `skeethook_crash.dmp` | мини-дамп для разбора в отладчике (если нашёлся `dbghelp.dll`) |
| буфер обмена | тот же текст, что в `.log` |

Куда смотреть в отчёте в первую очередь:

- **`на месте:`** — адрес падения. Внутри `hw.dll`/`client.dll` значит,
  что движку передали плохие данные; вне модулей или `0x000000xx` —
  разыменование мусорного указателя.
- **`операция:`** — чтение или запись. Запись по мусорному адресу почти
  всегда наша вина, чтение — часто движок.
- **`stage`** — докуда мод дошёл: `HUD_Redraw: ESP` = падение в
  отрисовке, `установка хуков` = ещё на старте.

Проверить, что механизм живой, можно без падения: вызвать экспорт
`sh_test_crash_report` — он запишет такой же отчёт с пометкой
«не краш».
