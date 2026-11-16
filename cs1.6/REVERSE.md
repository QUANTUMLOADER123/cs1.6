# Реверс CS 1.6 — что уже сделано

Файлы игры получены: `Core.dll`, `DemoPlayer.dll`, `SDL2.dll`, `client.dll`,
`client_mini.dll`, `hw.dll`, `mp.dll` (в корне репозитория).

## Что это за билд

Судя по импортам и строкам — это **Steam-версия** клиента:

| файл | что это | доказательство |
| --- | --- | --- |
| `hw.dll` | клиентский движок (рендер OpenGL + сеть) | импортирует `OPENGL32.dll` (20 функций), `WSOCK32.dll`, `steam_api.dll`; строки `cl_dlls\client.dll`, `HUD_VidInit`, `VClientVGUI001` |
| `client.dll` | клиентская игровая DLL (HUD, оружие, предсказание) | экспортирует `CreateInterface` + `F`, импортирует `SDL2.dll` и `steam_api.dll` |
| `mp.dll` | серверный GameDLL (правила, оружие, боты) | 372 экспорта, 3656 функций, MSVC |
| `Core.dll` | ядро лаунчера/обвязки | 792 функции |
| `DemoPlayer.dll` | проигрыватель демок | 402 функции |
| `client_mini.dll` | упрощённый клиент | 753 функции |

Все — PE32, i386, x86. ASLR/DEP выключены (`hw.dll`, `client.dll`) — типично для
старого GoldSrc-бинаря.

## Готовый результат: декомпиляция

`cs1.6/analysis/<dll>/` для каждой DLL:

```
decomp/         по файлу на функцию, с именем и адресом
symbols.txt     экспорты и импорты
vtables.txt     C++ vtables (классы)
strings.txt     все строки с адресами
index.json      машинный индекс (имя, адрес, размер, файл)
README.md       сводка: топ-100 функций по размеру
```

Цифры:

| DLL | функций | декомпилировано | размер выгрузки |
| --- | --- | --- | --- |
| hw.dll | 5630 | 4575 | ~22 МБ |
| client.dll | 3125 (после фильтра ≥16 байт) | 3125 | ~15 МБ |
| mp.dll | 3656 | 3656 | ~23 МБ |
| Core.dll | 792 | 792 | ~4.5 МБ |
| client_mini.dll | 753 | 753 | ~3.9 МБ |
| DemoPlayer.dll | 402 | 402 | ~1.8 МБ |

## Как это сделано

`tools/analysis/r2_decompile.py` — держит **один** процесс r2 в pipe-режиме через
PTY и гоняет `pdc` по каждой функции. Ключевой момент: обычные pipe'ы заставляют
r2 полностью буферизовать вывод, и диалог зависает; PTY даёт построчный обмен.
Скорость ~150 функций/сек против ~1/сек при запуске процесса на функцию.

```bash
python3 tools/analysis/r2_decompile.py hw.dll -o cs1.6/analysis/hw --min-size 16
```

## Что искать в декомпиляции

### Точки входа хуков

```bash
# экспорты-интерфейсы
grep -n "CreateInterface\|GiveFnptrsToDll\|GetEntityAPI" cs1.6/analysis/*/symbols.txt

# в hw.dll: HUD-функции, которые движок линкует из client.dll
grep -rn "HUD_Redraw\|HUD_Init\|CL_CreateMove" cs1.6/analysis/hw/strings.txt | head
```

### Готовые точки входа (client.dll)

`client.dll` экспортирует **все** HUD-функции — это ровно те адреса, которые движок
вызывает каждый кадр. Именно их использует любой нормальный чит для 1.6:

```
HUD_Redraw            0x???  — каждый кадр, после рендера мира
HUD_Frame             — тик клиента
HUD_PostRunCmd        — после обработки команды
HUD_AddEntity         — добавление сущности в кадр (ESP)
HUD_CreateEntities    — создание сущностей
HUD_GetHullBounds     — коллизии
HUD_DrawNormalTriangles / HUD_DrawTransparentTriangles  — хуки отрисовки
CL_CreateMove         — создание usercmd (aimbot, bhop)
```

Посмотреть все:

```bash
grep "HUD_\|CL_" cs1.6/analysis/client/symbols.txt
```

Это готовые цели для MinHook/DLL-инжекта: перехватил `HUD_Redraw` — получил
свой рендер-цикл с доступом ко всем мировым структурам.

### Работа со строками

```bash
grep -i "weapon_" cs1.6/analysis/client/strings.txt | head -20
grep -i "hud_" cs1.6/analysis/client/strings.txt | head -20
```

## Следующие шаги

1. **Именование функций.** В `cs1.6-src/cs16-client/` лежат исходники того же
   клиента — по совпадению строк и порядка вызовов можно переносить имена
   из исходника в декомпиляцию. Скрипт: `tools/analysis/rename_from_source.py`
   (ещё не написан).
2. **Поиск оффсетов.** Когда меню перейдёт в «софт» — нужны оффсеты
   `dwLocalPlayer`, `dwEntityList`, `dwViewMatrix`. Ищутся по паттернам из
   декомпиляции `hw.dll`.
3. **Хук рендера.** Уже есть рабочий `cs1.6/menu/build/menu_test_x86.dll`
   (MinHook на `wglSwapBuffers`).


## Где лежат выгрузки

Полная декомпиляция — `cs1.6/analysis/<dll>/decomp/`, в git **не входит**
(68 МБ текста). Восстанавливается за ~1 минуту на DLL:

```bash
python3 tools/analysis/r2_decompile.py hw.dll -o cs1.6/analysis/hw --min-size 16
```

В git лежат только `symbols.txt`, `vtables.txt`, `strings.txt`, `index.json`,
`README.md` — их хватает для навигации, а тело функций генерируется по запросу.
