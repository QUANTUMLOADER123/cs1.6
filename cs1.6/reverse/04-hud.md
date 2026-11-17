# Реверс CS 1.6 — HUD

Источники: `cl_dll/hud.h`, `cl_dll/hud.cpp` (692 строки),
`cl_dll/hud_redraw.cpp`, `cl_dll/hud_update.cpp`, `cl_dll/hud/hud_*.cpp`.

HUD — это слой `client.dll`, который рисует поверх мира. Один базовый класс,
23 наследника, связный список и порядок вызова.

---

## 1. Базовый класс `CHudBase`

Вся система строится на одном интерфейсе — `cl_dll/hud.h:123`:

```cpp
class CHudBase
{
public:
    int m_iFlags;                      // +0x00: HUD_ACTIVE / HUD_INTERMISSION
    virtual ~CHudBase();               // +0x04
    virtual int  Init();               // регистрация кваров и спрайтов
    virtual int  VidInit();            // загрузка спрайтов, зависит от видео
    virtual int  Draw(float flTime);   // отрисовка, каждый кадр
    virtual void Think();              // логика, каждый кадр
    virtual void Reset();              // сброс при смене карты
    virtual void InitHUDData();        // при подключении к серверу
    virtual void Shutdown();           // выгрузка
};
```

Порядок вызовов движком:

| момент | что зовётся |
|---|---|
| загрузка DLL | `HUD_Init` → `Init()` у всех |
| смена разрешения | `VidInit()` у всех |
| каждый кадр | `Think()` затем `Draw(flTime)` |
| смена карты | `Reset()` |
| коннект к серверу | `InitHUDData()` |
| выгрузка | `Shutdown()` |

Это **готовый контракт для своего элемента HUD**: наследуйся от `CHudBase`,
зарегистрируй в списке — движок сам начнёт вызывать.

---

## 2. Все 23 элемента

```
CHudBase
 ├── CHudAmmo           патроны: магазин/запас, иконка оружия
 ├── CHudAmmoSecondary  патроны второго ствола (M4 подствольник)
 ├── CHudBattery        броня (значок + число)
 ├── CHudDeathNotice    "X убил Y" в правом верхнем
 ├── CHudFlashlight     фонарик (иконка состояния)
 ├── CHudGeiger         счётчик радиации
 ├── CHudHealth         здоровье + крест
 ├── CHudMOTD           окно сообщения дня
 ├── CHudMenu           меню (выбор команды, buy menu)
 ├── CHudMessage        игровые сообщения по центру
 ├── CHudMoney           деньги ($)
 ├── CHudNVG            прибор ночного видения
 ├── CHudProgressBar    полоса прогресса
 ├── CHudRadio          меню радио-команд
 ├── CHudSayText        чат
 ├── CHudScenario       статус сценария
 ├── CHudScoreboard     таблица счёта (Tab)
 ├── CHudSniperScope    снайперский прицел (рендер)
 ├── CHudSpectator      вид от наблюдателя
 ├── CHudSpectatorGui   интерфейс наблюдателя
 ├── CHudStatusBar      строка статуса (иконки)
 ├── CHudStatusIcons    иконки статуса (бомба, defuse)
 ├── CHudTextMessage    текст сообщений
 ├── CHudTimer          таймер раунда
 └── CHudTrain          поезд
```

Файлы: `cl_dll/health.cpp`, `cl_dll/battery.cpp`, `cl_dll/ammo.cpp`,
`cl_dll/saytext.cpp`, `cl_dll/hud/hud_scoreboard.cpp`, `cl_dll/hud/money.cpp`,
`cl_dll/hud/radar.cpp`, `cl_dll/hud/timer.cpp`, `cl_dll/hud/nvg.cpp` и т.д.

---

## 3. Регистрация и связный список

```cpp
struct HUDLIST {          // cl_dll/hud.h:136
    CHudBase *p;          // +0x00
    HUDLIST  *pNext;      // +0x04
};
```

`CHud::m_pHudList` — голова списка. Обход в `hud_redraw.cpp:50`:

```cpp
for (HUDLIST *pList = m_pHudList; pList; pList = pList->pNext)
    pList->p->Draw(flTime);
```

**Порядок в списке = порядок отрисовки = z-порядок.** Последний элемент
рисуется поверх всех.

### Флаги `m_iFlags`

```c
#define HUD_ACTIVE          0x0001   // вызывать Think/Draw
#define HUD_INTERMISSION    0x0002   // скрыть во время интермиссии
#define HUD_DRAW            0x0004   // рисовать принудительно
```

---

## 4. Порог отрисовки

`hud_redraw.cpp` проверяет, нужно ли вообще рисовать:

```c
if (gHUD.m_iHideHUDDisplay & HIDEHUD_ALL) return;
```

Маски скрытия:

| флаг | что скрывает |
|---|---|
| `HIDEHUD_WEAPONS` | патроны, оружие |
| `HIDEHUD_FLASHLIGHT` | фонарик |
| `HIDEHUD_ALL` | всё |
| `HIDEHUD_HEALTH` | здоровье, броня |
| `HIDEHUD_TIMER` | таймер |
| `HIDEHUD_MONEY` | деньги |
| `HIDEHUD_CROSSHAIR` | прицел |

Это же маска, которую использует **спектатор** и **снайперский прицел**.
Чит, который «хочет всё видеть», просто снимает биты из `gHUD.m_iHideHUDDisplay`.

---

## 5. Масштабирование

Всё в HUD позиционируется через `ScreenWidth`/`ScreenHeight` с коэффициентом:

```c
float Scale = ScreenHeight / 480.0f;   // базовая высота — 480
```

Поэтому HUD **пропорционален** разрешению. Твоя своя отрисовка должна делать
то же, иначе на 16:9 элементы уедут.

---

## 6. Цвета команд

`cl_dll/hud.cpp:53` — точные значения:

```c
{ 204, 204, 204 },   // наблюдатели — серый
{ 255,  64,  64 },   // CT — красный
{ 153, 204, 255 },   // T  — голубой
```

Заметь: **CT красные, T голубые**. Это CS 1.6, тут всё наоборот относительно
того, что ожидают от CS:GO.

---

## 7. Как HUD получает данные

Три канала:

1. **`HUD_UpdateClientData`** (`0x01944A80`) — движок отдаёт
   `client_data_t` (здоровье, броня, патроны, оружие). Раз в кадр.
2. **User messages** — `pfnHookUserMsg` (`gEngfuncs+0x48`). События вроде
   `TextMsg`, `Health`, `Battery`, `WeaponList`, `ScoreAttrib`.
3. **Entity state** — `cl_entity_t` из `cl_entities`, для позиций в мире.

`HUD_UpdateClientData` возвращает 1, если данные изменились — движок тогда
перерисовывает.

---

## 8. Точки входа в `client.dll`

| VA | экспорт |
|---|---|
| `0x019449D0` | `HUD_Init` — создаёт все элементы и зовёт `Init()` |
| `0x01944A60` | `HUD_Redraw` — обход списка, `Draw()` |
| `0x01944A80` | `HUD_UpdateClientData` |
| `0x019449C0` | `HUD_VidInit` — `VidInit()` у всех |
| `0x01944AA0` | `HUD_Reset` |
| `0x019449E0` | `HUD_Shutdown` |
| `0x01958330` | `HUD_Key_Event` — клавиши |
| `0x019461D0` | `HUD_TxferPredictionData` |

Сигнатуры (уникальные, проверены по бинарнику):

```
HUD_Init      8B 44 24 08 81 EC 08 04 00 00 B9 87 00 00 00 83 F8 07 53 56 8B B4
```

---

## 9. Что важно для ESP

ESP рисует **поверх** HUD или **под** ним — зависит от точки вставки:

- **после** обхода списка → поверх всего (обычный ESP)
- **до** обхода → ESP окажется под HUD (лучше для читаемости по краям)

Данные для ESP берутся из `cl_entity_t`:

```c
cl_entity_t *ent = gEngfuncs.GetEntityByIndex(i);
if (!ent || !ent->player || ent->curstate.health <= 0) continue;

// проекция 3D → 2D
int x, y;
gEngfuncs.pfnCalcScreen(ent->origin, &x, &y);   // gEngfuncs+0x???
```

Проверка «виден ли» — `pfnTriAPI->WorldToScreen` + трассировка через
`pmove->PM_PlayerTrace` (`playermove_t+0x...)`.

---

## 10. Чат и сообщения

`CHudSayText` (`saytext.cpp`) обрабатывает `SayText` user message.
Формат — имя, двоеточие, текст. Перехват через `pfnHookUserMsg("SayText", ...)`
даёт доступ ко всем сообщениям в чате — основа для чат-чита.

`CHudTextMessage` — `TextMsg` (системные сообщения), ключи локализации
`#Game_...`.
