# Фулл-реверс CS 1.6 до исходников

Реверс клиента, движка и серверной логики до уровня, при котором из
восстановленных контрактов можно писать код, а не «читать память по оффсетам».

## Документы

| файл | что внутри |
|---|---|
| [`01-movement.md`](01-movement.md) | физика: трение, разгон, air-strafe cap 30, bhop 1.2/0.8, гравитация, коллизия |
| [`02-weapons.md`](02-weapons.md) | 29 стволов: скорость, урон, три класса разброса, отдача, `FireBullets3` |
| [`03-interfaces.md`](03-interfaces.md) | `cl_enginefunc_t` (540 Б), `enginefuncs_t` (632 Б), `playermove_t`, `usercmd_t` |
| [`04-hud.md`](04-hud.md) | `CHudBase`, 23 элемента, связный список, флаги, порядок отрисовки |
| [`05-rules-server.md`](05-rules-server.md) | происхождение бинарников, RTTI 651 класс, `AddToFullPack`, правила, боты |
| [`06-networking.md`](06-networking.md) | `entity_state_t` 340Б, `cl_entity_t` 2996Б, 150 user messages |
| [`07-render.md`](07-render.md) | `triangleapi_t`, `WorldToScreen`, `BoxInPVS`, порядок кадра |
| [`08-events.md`](08-events.md) | `event_api_t`, `efx_api_t`, скрипты `.sc`, трассеры |
| [`09-chams.md`](09-chams.md) | `CStudioModelRenderer`, 128 костей, чеймы, хитбоксы |
| [`10-anticheat.md`](10-anticheat.md) | что реально видит VAC, границы SDK |
| [`11-clientdata.md`](11-clientdata.md) | `clientdata_t` 476 Б, `weapon_data_t` 60 Б, конвейер кадра |
| [`12-ballistics.md`](12-ballistics.md) | `random_seed`, ГПСЧ, разброс, отдача, автоволл, autostrafe |
| [`13-hud-layout.md`](13-hud-layout.md) | HUD в 640×480, `XRES`/`YRES`, 23 элемента, `.dem` |
| [`14-grenades.md`](14-grenades.md) | формула броска, гравитация 800, оптимум 42°, урон |
| [`15-antiaim.md`](15-antiaim.md) | **magic pitch 1620**, movement fix, fake lag, правда про desync |
| [`16-feature-map.md`](16-feature-map.md) | **карта всех фич**: что работает, чем, почему нет остального |
| [`17-svc-protocol.md`](17-svc-protocol.md) | **59 opcodes `svc_*`** в точном порядке протокола |
| [`18-weapon-timers.md`](18-weapon-timers.md) | `GetNextAttackDelay`, attack creep, fast reload |
| [`19-move-prediction.md`](19-move-prediction.md) | **predecition движения**: friction, accelerate, air-кап 30, система координат |
| [`20-weapon-data.md`](20-weapon-data.md) | **`weapon_data_t`**: 64 оружия, предсказанные таймеры |
| [`21-clientdll-slots.md`](21-clientdll-slots.md) | **32 слота экспортов `client.dll`** — точные адреса без паттернов |
| [`22-bones-studio.md`](22-bones-studio.md) | **кости** через `HUD_GetStudioModelInterface` |
| [`23-client-overrides.md`](23-client-overrides.md) | `iuser1..4`: три разные семантики в трёх функциях, `iOnTrain` |
| [`24-rage-limits.md`](24-rage-limits.md) | границы GoldSrc: double tap, hideshot, desync, roll — что реально возможно |
| [`SIGNATURES.md`](SIGNATURES.md) | таблица сигнатур — все адреса без хардкода |

Полные таблицы экспорта: [`exports/`](exports/) — 323 у `hw.dll`,
87 у `client.dll`, 372 у `mp.dll`.

## Происхождение бинарников (важнейший вывод)

```
client.dll   C:\buildslave\goldsrc_win32\...\cstrike\cl_dll\Release\client.pdb
hw.dll       ...\GoldSrc\engine\GL_Release_STEAM\hw.pdb
mp.dll       ReGameDLL version: 5.23.0.620-dev  /  commit ac07e89
```

Клиент и движок — **ванильная сборка Valve**. Поэтому реверс-исходники
`FWGS/cs16-client` описывают их точно. Проверено совпадением `sizeof` и
оффсетов:

| структура | из заголовка | из бинарника | |
|---|---:|---:|---|
| `cl_enginefunc_t` | 540 | `rep movs ecx=0x87` = 540 | ✅ |
| `enginefuncs_t` | 632 | `rep movs ecx=0x9E` = 632 | ✅ |
| `usercmd_t` | 52 | `rep stos ecx=0xD` = 52 | ✅ |
| `physent_t` | 224 | код `i*7<<5` = 224 | ✅ |
| `playermove_t` | `0x4F6BC` | 600 physent + 64 moveent + 600 visent + 600 pmtrace (было ошибочно 1508) | ✅ |
| `INTERFACE_VERSION` | 7 | `cmp eax,7` в `Initialize` | ✅ |
| `entity_state_t` | 340 | `cl+0x364` weaponmodel читается кодом | ✅ |
| `cl_entity_t` | 2996 | `cl+0xB54/0xB58/0xB5C` = angles.x/y/z | ✅ |

## Ключевые адреса (найдены и проверены)

```
client.dll  ImageBase 0x01900000
  0x01944710  Initialize              версия 7, копия 135 указателей
  0x01963050  HUD_PlayerMove (тело)   g_pmove @ 0x01A36D84
  0x01958CC0  CL_CreateMove           memset(cmd,0,52)

mp.dll      ImageBase 0x10000000
  0x100539F0  GiveFnptrsToDll         158 указателей движка
  0x100E2B10  weapon_ak47
```

## Инструменты

| файл | назначение |
|---|---|
| `tools/analysis/pe_exports.py` | дамп экспорта/импорта PE → `exports/` |
| `tools/analysis/struct_layout.py` | раскладка структур из заголовков + сверка с бинарником |
| `tools/analysis/xref.py` | поиск строки и кросс-ссылок на неё |
| `tools/analysis/sig_gen.py` | генерация и проверка уникальности сигнатур |

## Как повторять

```bash
# экспорт всех бинарников
python3 tools/analysis/pe_exports.py hw.dll mp.dll client.dll --imports -o cs1.6/reverse/exports

# раскладка + сверка
python3 tools/analysis/struct_layout.py --header /home/user/cs1.6-ref/pm_shared/pm_defs.h \
    --struct playermove_t --check "playermove_t.flags=0xB8" "playermove_t.friction=0xC4"

# сигнатуры
python3 tools/analysis/sig_gen.py client.dll --min 22 --at 0x1944710 0x1963050
```

## Следующее

- `08-events.md` — `PlaybackEvent`, `events/*.sc`, партиклы
- `09-chams.md` — `StudioDrawModel`, подмена `rendermode`
- сверка клиентских сигнатур между версиями сборки

---

## Результат реверса: SDK

Реверс применён на практике — [`../skeethook_1_6/`](../skeethook_1_6/):
SDK + чит, где **все адреса добываются сигнатурами**, а структуры взяты
из этого реверса. Проверено:

- 5 сигнатур уникальны в `client.dll`, находят точные адреса;
- `gEngfuncs` извлекается по `imm32` → `0x01A25CC0` (совпало);
- `g_pmove` извлекается по `imm32` → `0x01A36D84` (совпало);
- `sizeof(cl_enginefunc_t) = 0x21C` подтверждён компилятором.
