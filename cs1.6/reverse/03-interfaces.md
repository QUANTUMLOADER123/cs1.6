# Реверс CS 1.6 — интерфейсы движка

Как три бинарника разговаривают друг с другом. Это фундамент: любой модуль,
который хочет лезть в игру, входит именно здесь.

---

## 1. Три модуля и их роли

| файл | размер | экспортов | роль |
|---|---:|---:|---|
| `hw.dll` | 1 641 376 | **323** | движок: рендер, звук, ввод, файлы, сеть; экспортирует VGUI-классы |
| `client.dll` | 1 096 608 | **87** | клиент: HUD, ввод, предсказание, интерполяция |
| `mp.dll` | 1 714 176 | **372** | игровая логика (сервер + оружие + правила + боты) |

Полные таблицы — `cs1.6/reverse/exports/`.

---

## 2. `client.dll` ↔ движок: версия 7, 135 функций

Движок загружает `client.dll`, вызывает `Initialize` и передаёт таблицу
функций. Дизассемблер `Initialize` по адресу `0x01944710`:

```asm
01944710  mov  eax,[esp+0x8]        ; pfnEngineFuncs (второй аргумент)
01944714  sub  esp,0x408
0194471a  mov  ecx,0x87             ; 135 dword
0194471f  cmp  eax,0x7              ; ⭐ требуется версия интерфейса 7
0194472c  mov  edi,0x1a25cc0        ; ⭐ gEngfuncs — глобальная копия
01944731  rep movs                  ; копируем 135*4 = 540 байт
```

**Три вещи, подтверждённые одновременно:**

1. `INTERFACE_VERSION = 7`
2. `sizeof(cl_enginefunc_t) = 540` (135 указателей) — ровно как в заголовке
3. Копия таблицы лежит в `client.dll` по **`client.dll+0x125CC0`** (VA `0x1A25CC0`)

Если движок объявит другую версию — `Initialize` вернёт 0 и игра не загрузится.

---

## 3. `cl_enginefunc_t` — что по каким оффсетам

Базовый адрес таблицы: **`client.dll + 0x125CC0`**.

### Ввод / углы

| оффсет | функция | зачем |
|---|---|---|
| `+0x0088` | `GetViewAngles(vec3)` | прочитать углы обзора |
| `+0x008C` | `SetViewAngles(vec3)` | записать углы (anti-aim, silent) |
| `+0x00C0` | `Key_Event(int, int)` | эмуляция нажатия клавиши |
| `+0x009C` | `Cmd_Argv(int)` | аргументы команды |
| `+0x004C` | `pfnServerCmd(char*)` | надёжная команда на сервер |
| `+0x0184` | `pfnServerCmdUnreliable(char*)` | ненадёжная |
| `+0x0050` | `pfnClientCmd(char*)` | выполнить клиентскую команду |
| `+0x0218` | `pfnFilteredClientCmd` | как `ClientCmd`, но через фильтр |

### Вывод на экран

| оффсет | функция |
|---|---|
| `+0x0078` | `pfnConsolePrint` |
| `+0x00A0` | `Con_Printf` |
| `+0x00A4` | `Con_DPrintf` |
| `+0x006C` | `pfnDrawConsoleString` |
| `+0x0074` | `pfnDrawConsoleStringLen` |

### Квары

| оффсет | функция |
|---|---|
| `+0x0094` | `Cvar_SetValue(char*, float)` |
| `+0x01E4` | `Cvar_Set(char*, char*)` |

### Игрок и мир

| оффсет | функция |
|---|---|
| `+0x00CC` | `GetLocalPlayer()` → индекс локального игрока |
| `+0x00D0` | `GetViewModel()` → указатель на модель в руках |
| `+0x0054` | `pfnGetPlayerInfo(int, hud_player_info_t*)` |
| `+0x0030` | `pfnGetScreenInfo(SCREENINFO*)` — размеры экрана |
| `+0x0048` | `pfnHookUserMsg` — перехват сетевых сообщений |

### Звук / случайность

| оффсет | функция |
|---|---|
| `+0x0058` | `pfnPlaySoundByName` |
| `+0x005C` | `pfnPlaySoundByIndex` |
| `+0x00FC` | `pfnPlaySoundByNameAtLocation` |
| `+0x0204` | `pfnPlaySoundByNameAtPitch` |
| `+0x010C` | `pfnRandomFloat` |
| `+0x0110` | `pfnRandomLong` |

---

## 4. Скелет `client.dll` — 87 экспортов

Главные:

| VA | экспорт | роль |
|---|---|---|
| `0x01944710` | `Initialize` | вход, версия 7 |
| `0x019449D0` | `HUD_Init` | инициализация клиента |
| `0x01958CC0` | `CL_CreateMove` | **вызывается сервером через сеть, строит usercmd** |
| `0x01944620` | `HUD_PlayerMove` | тунт → `0x01963050` |
| `0x01944600` | `HUD_PlayerMoveInit` | тунт → `0x01963120` |
| `0x01944610` | `HUD_PlayerMoveTexture` | тунт → `0x0195D9B0` |
| `0x01944A60` | `HUD_Redraw` | отрисовка кадра |
| `0x01944A80` | `HUD_UpdateClientData` | синхронизация клиента |
| `0x01946010` | `HUD_ProcessPlayerState` | применение состояния игрока |
| `0x01945FC0` | `HUD_TxferLocalOverrides` | локальные переопределения |
| `0x019461D0` | `HUD_TxferPredictionData` | данные предсказания |
| `0x0190EC40` | `HUD_PostRunCmd` | после исполнения команды |
| `0x01958330` | `HUD_Key_Event` | клавиши |
| `0x01946E40` | `HUD_GetUserEntity` | |
| `0x0194A840` | `HUD_GetStudioModelInterface` | |

Заметь: **экспорты — это тунты** (`jmp` на тело). Например `HUD_PlayerMove`
(`0x01944620`) = `jmp 0x01963050`. Реальный код живёт по второму адресу.

---

## 5. `mp.dll` ↔ движок: `enginefuncs_t`, 158 функций

```asm
100539F0  GiveFnptrsToDll:              ; экспорт #1
10053A04    mov esi,[ebp+0x8]           ; pefunctionstbl
10053A15    mov ecx,0x9E                ; 158 dword
10053A1A    mov edi,0x101B0BC0          ; глобальная копия
10053A24    rep movs                    ; 158*4 = 632 байта
```

**`sizeof(enginefuncs_t) = 632`** (158 указателей) — точно совпало с
посчитанным из `engine/eiface.h`.

`mp.dll` получает **в 1.17 раза больше** функций, чем клиент: серверу нужны
спавн сущностей, трассировки, звуки, работа с edict'ами.

### Прочие структуры

| структура | размер | где |
|---|---:|---|
| `enginefuncs_t` | 632 | `mp.dll` `+0x1B0BC0` |
| `DLL_FUNCTIONS` | 200 | `engine/eiface.h:500` |
| `NEW_DLL_FUNCTIONS` | 20 | `engine/eiface.h:516` |
| `cl_enginefunc_t` | 540 | `client.dll+0x125CC0` |

---

## 6. `playermove_t` — структура физики

`HUD_PlayerMoveInit` возвращает указатель на неё движку. Раскладка проверена
по бинарнику (`client.dll`, `HUD_PlayerMove` @ `0x01963050`):

```asm
0196305a  mov ds:0x1a36d84, eax        ; ⭐ g_pmove — глобальный playermove_t*
01963072  mov ecx,[eax+0xe0]           ; onground
0196307b  mov ecx,[eax+0xb8]           ; flags
01963083  or  ch,0x02                  ;   → FL_ONGROUND (0x200)
01963088  and ch,0xfd                  ;   → снять FL_ONGROUND
019630a6  mov [eax+0xc4], 0x3f800000   ; friction = 1.0f
```

| оффсет | поле | подтверждение |
|---|---|---|
| `+0x00B8` | `flags` | `[eax+0xb8]` |
| `+0x00C4` | `friction` | `[eax+0xc4] = 1.0f` |
| `+0x00DC` | `movetype` | `[eax+0xdc] == 3` |
| `+0x00E0` | `onground` | `[eax+0xe0]`, сравнение с `-1` |
| `+0x024C` | `numphysent` | `[ecx+0x24c]` |
| `+0x0250` | `physents[]` | `[ecx+0x250]` |

**`sizeof(playermove_t) = 325308` (0x4F6BC)**, **`sizeof(physent_t) = 224`** (код считает
адрес как `i*7 << 5` = `i*224`), **`sizeof(pmtrace_t) = 68` (0x44)**.

Раньше здесь стояло `1508`. Это ошибка, и вот почему она
получилась: 1508 — это размер **первых 0x250 байт** (`physents`
начинается ровно на `0x250`, а `0x250 + 4 + 48 + 48` как раз даёт
похожее число). Структура не кончается на `physents` — дальше идут
`moveents[64]`, `visents[600]`, `touchindex[600]` и ещё 25 указателей.

Проверка сходится до байта, если посчитать от подтверждённых оффсетов
(`physents` начинается на `0x250`, подтверждено кодом):

```
0x250                         начало physents[]
+ 600 * 0xE0  = 0x20F50       physents[600]
+ 4 + 64 * 0xE0 = 0x24754     nummoveent + moveents[64]
+ 4 + 600 * 0xE0 = 0x45458    numvisent + visents[600]
+ 0x34 + 4                    cmd + numtouch
+ 600 * 0x44   = 0x4F3F0      touchindex[600]  (pmtrace_t = 68)
+ 512                         physinfo[MAX_PHYSINFO_STRING]
+ 4 + 48 + 48                 movevars + player_mins + player_maxs
+ 26 * 4       = 0x68         26 указателей PM_Info_ValueForKey..PM_TraceLineEx
  = 325308 = 0x4F6BC
```

Совпадает с `sizeof` от компилятора **точно**.

Четыре ловушки, на которых легко ошибиться:

1. **`pmtrace_t` = 68 (0x44), а не 64.** Внутри есть `vec3_t deltavelocity`
   между `ent` и `hitgroup`, и `pmplane_t plane` = `{vec3_t normal; float dist;}`
   (16 байт), а не набор из пяти полей.
2. **`physinfo` = 512 байт.** `MAX_PHYSINFO_STRING` в `pm_info.h:35`
   равен 256, но это для `entity_state_t`. В `playermove_t` поле занимает
   512 — при 256 размер не сходится ровно на 256.
3. **`qboolean` = `int` (4 байта),** а не `bool`. `common/const.h:742`.
4. **Указателей-членов 26.** В `pm_defs.h` между ними вкраплены
   параметры `pfnIgnore` (в `PM_PlayerTraceEx` и т.д.) — если считать
   все вхождения `(*`, получится 29 и размер уедет на 12 байт.

Обратите внимание: `MAX_MOVEENTS` = 64, а не 600 — тоже была ошибка,
из-за которой структура не сходилась.

### `FL_` флаги (подтверждены кодом)

```c
FL_ONGROUND  = 0x200    // установка через «or ch, 2» при байте [eax+0xB9]
FL_DUCKING   = 0x4
FL_WATERJUMP = 0x800
```

---

## 7. `usercmd_t` — 52 байта

`CL_CreateMove` по адресу `0x01958CC0`:

```asm
01958D15  mov ecx,0xd          ; 13 dword
01958D1a  xor eax,eax
01958d1e  rep stos             ; memset(cmd, 0, 52)
```

`sizeof(usercmd_t) = 52` — совпадает с раскладкой из `common/usercmd.h`:

```c
short lerp_msec;    // +0x00
byte  msec;         // +0x02
                    // +0x03: выравнивание
vec3_t viewangles;  // +0x04 .. +0x0F
float forwardmove;  // +0x10
float sidemove;     // +0x14
float upmove;       // +0x18
byte  lightlevel;   // +0x1C
unsigned short buttons; // +0x1E
byte  impulse;      // +0x20
byte  weaponselect; // +0x21
                    // +0x22: выравнивание
int   impact_index; // +0x24
vec3_t impact_position; // +0x28 .. +0x33  → всего 52 (0x34)
```

---

## 8. `movevars_t` — параметры физики

26 полей, `pm_shared/pm_movevars.h`. Именно сюда сервер пишет значения
`sv_gravity`, `sv_maxspeed`, `sv_friction` и т.д. Указатель лежит в
`playermove_t->movevars`.

Важно: **имена полей не совпадают с именами кваров** — см. таблицу в
`01-movement.md`. В самих исходниках `cs16-client` серверных кваров нет,
значения надо брать из `hw.dll` или из дефолтов.

---

## 9. Иерархия классов из `mp.dll`

В бинарнике есть RTTI-имена (`CBasePlayerWeapon` — 40 вхождений):

```
CBaseEntity
  └── CBasePlayerItem
        └── CBasePlayerWeapon
              ├── CAK47        (0x100E2B10, RTTI 0x1017FF38)
              ├── CM4A1        (0x100ECF20)
              ├── CAWP, CDEAGLE, CGLOCK18, ...
              └── (29 стволов)
CWeaponBox                          (0x100A41D0 BombThink, 0x100A4440 Kill)
```

Полная карта оружия — см. `MAPPING.md` в этой папке.

---

## 10. Практический вход

Чтобы что-то сделать в игре, нужны три адреса:

```
1. база модуля      GetModuleHandle("client.dll")
2. gEngfuncs        client.dll + 0x125CC0   (540 Б, 135 указателей)
3. g_pmove          client.dll + 0x136D84   (playermove_t*, 0x4F6BC Б)
```

Через `gEngfuncs` доступны углы (`+0x88`/`+0x8C`), квары (`+0x94`),
локальный игрок (`+0xCC`) и вывод (`+0x78`/`+0xA0`).
Через `g_pmove` — вся физика: скорость (`+0x74`), флаги (`+0xB8`),
присед (`+0xBC`), трение (`+0xC4`).

> Замечание: `client.dll+0x136D84` — адрес из этой конкретной сборки
> (`ImageBase 0x01900000`). При другой версии искать по сигнатуре: код
> `mov ds:XXXXXXXX, eax` в начале `HUD_PlayerMove`.
