# Реверс CS 1.6 — раскладка HUD и демки

Источник: `cl_dll/cl_util.h`, `cl_dll/hud.cpp`, `cl_dll/hud.h`,
`common/demo_api.h`.

---

## 1. Главное: HUD всегда в 640×480 ⭐

```c
// cl_util.h:74
#define XRES(x) ((int)(float(x) * ((float)ScreenWidth / 640.0f) + 0.5f))
#define YRES(y) ((int)(float(y) * ((float)ScreenHeight / 480.0f) + 0.5f))

#define ScreenHeight (gHUD.m_scrinfo.iHeight)
#define ScreenWidth  (gHUD.m_scrinfo.iWidth)
```

```c
// hud.cpp:474
m_iRes = 640;
```

**Все координаты элементов HUD заданы в виртуальном разрешении 640×480**
и масштабируются через `XRES`/`YRES`. Это значит:

- `XRES` — не линейное масштабирование: `+0.5f` округляет, поэтому на
  1920 ширине `XRES(10) = 30`, а не 30.0;
- пропорции **не** сохраняются при нестандартном соотношении: 16:9
  растягивает по X сильнее, чем по Y (потому что `XRES` делит на 640, а
  `YRES` на 480);
- если рисуешь свой ESP по пикселям экрана — он «поедет» относительно HUD
  на не-4:3. Правильно — тоже считать в 640×480 и применять `XRES`/`YRES`.

### Проверка масштаба

```c
// hud.cpp:472
m_flScale = (float)TrueWidth / (float)ScreenWidth;
```

`TrueWidth` — из `vid_width`, `ScreenWidth` — реальная. Отношение даёт
компенсацию при рендере в буфер меньше экрана (как в оконном режиме).

---

## 2. Полный список HUD-элементов

Из `hud.h` — **23 класса**, каждый наследует `CHudBase`:

| класс | файл | что рисует |
|---|---|---|
| `CHudAmmo` | `ammo.cpp` | патроны + иконка оружия |
| `CHudAmmoSecondary` | `ammo_secondary.cpp` | запасные патроны |
| `CHudBattery` | `battery.cpp` | броня + шлем |
| `CHudHealth` | `health.cpp` | здоровье + крест смерти |
| `CHudDamage` | `health.cpp` | индикатор урона (стрелки) |
| `CHudGeiger` | `geiger.cpp` | счётчик Гейгера |
| `CHudTrain` | `train.cpp` | скорость поезда |
| `CHudMOTD` | `motd.cpp` | окно MOTD |
| `CHudScoreboard` | `scoreboard.cpp` | таблица счёта |
| `CHudStatusBar` | `statusbar.cpp` | статус-бар |
| `CHudDeathNotice` | `deathnotice.cpp` | killfeed |
| `CHudMenu` | `menu.cpp` | меню покупки |
| `CHudSayText` | `saytext.cpp` | чат |
| `CHudMessage` | `message.cpp` | центр-сообщения |
| `CHudStatusIcons` | `status_icons.cpp` | иконки статуса |
| `CHudMoney` | `money.cpp` | деньги |
| `CHudRadio` | `radio.cpp` | радио-меню |
| `CHudTimer` | `timer.cpp` | таймер раунда |
| `CHudProgressBar` | `progressbar.cpp` | прогресс (C4) |
| `CHudSniperScope` | `SniperScope.cpp` | прицел снайперки |
| `CHudNVG` | `nvg.cpp` | ночное видение |
| `CHudScenario` | `ScenarioStatus.cpp` | статус сценария |
| `CHudSpectator` | `hud_spectator.h` | наблюдение |

### `CHudBase`

```c
class CHudBase
{
public:
    virtual ~CHudBase();
    int   m_iFlags;         // +0x00
    // +0x04 — dtor
    virtual int  Init(void);
    virtual int  VidInit(void);
    virtual int  Draw(float time);
    virtual void Think(void);
    virtual void Reset(void);
    virtual void InitHUDData(void);
};
```

Флаги:

```c
#define HUD_ACTIVE       1
#define HUD_INTERMISSION 2
#define HUD_DRAW         4
```

`HUD_DRAW` — рисуется каждый кадр, `HUD_ACTIVE` — только по `Think`.
Именно `m_iFlags` надо править, чтобы «включить» элемент,
который игра выключила (например, `CHudTimer` на серверах без таймера).

---

## 3. Порядок инициализации HUD

```
HUD_Init (0x19449D0)                  cl_dll/cdll_int.cpp:274
  ├── LoadMenuInterface()
  ├── InitInput()
  └── gHUD.Init()
        ├── m_iRes = 640
        ├── каждый элемент -> Init()
        └── gHUD.VidInit()
              ├── GetScreenInfo(&m_scrinfo)
              ├── m_truescrinfo = vid_width/vid_height
              ├── m_flScale = TrueWidth / ScreenWidth
              ├── SPR_GetList("sprites/hud.txt")
              └── каждый элемент -> VidInit()
```

`VidInit` вызывается **при смене разрешения** — на нём и надо
пересчитывать свои элементы.

---

## 4. Точки перехвата

| что | как | зачем |
|---|---|---|
| `HUD_Redraw` | хук (уже есть) | своя отрисовка поверх |
| `CHud::VidInit` | хук на `HUD_Init` | узнать смену разрешения |
| `m_iFlags` элемента | прямая запись | включить скрытый элемент |
| `CHudBase::Draw` | vtable-хук | заменить отрисовку элемента |

Пример включения таймера, который игра прячет:

```c
typedef struct
{
    void **vtable;
    i32 flags;
} sh_hud_base_t;

void sh_hud_force_active(void *element)
{
    if (!element) return;
    i32 *flags = (i32 *)((u8 *)element + 4);
    *flags |= 1 | 4;
}
```

⚠️ `+0x04` — второй слот после vtable: `CHudBase` не имеет полей кроме
`m_iFlags`, поэтому `flags` лежит сразу за vtable, **а dtor — в vtable**.
Проверка в `hud.h:123` этого не показывает явно — там виртуальный dtor
объявлен первым, но vptr в MSVC идёт первым полем, значит `m_iFlags`
на `+0x04`.

---

## 5. Демки: `demo_api_t`

```c
// common/demo_api.h
typedef struct demo_api_s
{
    int  (*IsRecording)(void);
    int  (*IsPlayingback)(void);
    int  (*IsTimeDemo)(void);
    void (*WriteBuffer)(int size, unsigned char *buffer);
} demo_api_t;
```

Всего **4 функции**. Это не «чтение демки», а поток записи:

- `IsRecording()` — идёт ли запись;
- `IsPlayingback()` — проигрывается ли демка;
- `IsTimeDemo()` — это `timedemo` (замер FPS);
- `WriteBuffer(size, buf)` — **вызывается движком**, когда нужно записать
  блок в файл демки.

Значит: клиент не «разбирает» демку, он **получает её содержимое через
`WriteBuffer`**, когда движок пишет. Для анализа демки нужно самому
парсить формат `.dem`.

---

## 6. Формат `.dem` GoldSrc

```
заголовок:
  char  magic[8]      = "HLDEMO\0"
  int   demo_protocol
  int   net_protocol
  char  mapname[260]
  char  gamedir[260]
  int   mapcrc
  int   dirsOffset     <- смещение таблицы директорий

далее — блоки:
  int   type            (1..6, 0 = конец)
  float time
  int   frame
  int   size
  byte  data[size]
```

Типы блоков:

| type | имя | содержимое |
|---:|---|---|
| 0 | `dem_stop` | конец файла |
| 1 | `dem_file` | имя записываемого файла |
| 2 | `dem_sound` | звуковые события |
| 3 | `dem_cmd` | `usercmd_t` (то, что нажимал игрок) |
| 4 | `dem_net` | **сетевой пакет** — главное |
| 5 | `dem_strings` | отправка строк (в начале) |
| 6 | `dem_buffer` | сырой буфер |

**`dem_net` — это поток `svc_*` сообщений**, тот же протокол, что
описан в `06-networking.md`. Значит: зная `svc_*` opcodes, можно
восстановить позиции, оружие, выстрелы из демки — то есть сделать
**полноценный парсер демок**, независимый от игры.

Порядок «строка 1» — `dem_strings`/`dem_file`, потом `dem_net`.

---

## 7. Что это даёт SDK

| фича | как |
|---|---|
| spectator list | `demo_api->IsPlayingback()` — если играем демку |
| запись своих демок | `IsRecording()` |
| индикатор timedemo | `IsTimeDemo()` |
| **анализ демок** | парсинг `.dem` через `dem_net` |
| HUD-скалирование | `XRES`/`YRES` для своих элементов |
| включить скрытый элемент | `m_iFlags |= HUD_DRAW` |

Последнее особенно полезно: `CHudTimer` на сервере без таймера просто
не рисуется, но объект **создан** — флаг включает его без хака логики.

---

## 8. Следующее

- `14-demo-format.md` — полный разбор `svc_*` для парсера демок
- перенос ESP на `XRES`/`YRES`
